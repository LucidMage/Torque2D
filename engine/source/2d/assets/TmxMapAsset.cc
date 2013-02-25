#include "TmxMapAsset.h"

#include "2d/scene/SceneRenderQueue.h"

// Debug Profiling.
#include "debug/profiler.h"

#include "TmxMapAsset_ScriptBinding.h"

//------------------------------------------------------------------------------

ConsoleType( tmxMapAssetPtr, TypeTmxMapAssetPtr, sizeof(AssetPtr<TmxMapAsset>), ASSET_ID_FIELD_PREFIX )

//-----------------------------------------------------------------------------

ConsoleGetType( TypeTmxMapAssetPtr )
{
	// Fetch asset Id.
	return (*((AssetPtr<TmxMapAsset>*)dptr)).getAssetId();
}

//-----------------------------------------------------------------------------

ConsoleSetType( TypeTmxMapAssetPtr )
{
	// Was a single argument specified?
	if( argc == 1 )
	{
		// Yes, so fetch field value.
		const char* pFieldValue = argv[0];

		// Fetch asset pointer.
		AssetPtr<TmxMapAsset>* pAssetPtr = dynamic_cast<AssetPtr<TmxMapAsset>*>((AssetPtrBase*)(dptr));

		// Is the asset pointer the correct type?
		if ( pAssetPtr == NULL )
		{
			// No, so fail.
			Con::warnf( "(TypeTmxMapAssetPtr) - Failed to set asset Id '%d'.", pFieldValue );
			return;
		}

		// Set asset.
		pAssetPtr->setAssetId( pFieldValue );

		return;
	}

	// Warn.
	Con::warnf( "(TypeTmxMapAssetPtr) - Cannot set multiple args to a single asset." );
}

//------------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(TmxMapAsset);

//------------------------------------------------------------------------------

/// Custom map taml configuration
static bool explicitCellPropertiesInitialized = false;

static StringTableEntry			layerCustomPropertyName;
static StringTableEntry			layerAliasName;
static StringTableEntry			layerIdName;
static StringTableEntry			layerIndexName;


//------------------------------------------------------------------------------

TmxMapAsset::TmxMapAsset() :  mMapFile(StringTable->EmptyString),
	mParser(NULL)

{
	if (!explicitCellPropertiesInitialized)
	{
		layerCustomPropertyName = StringTable->insert("Layers");
		layerAliasName = StringTable->insert("Layer");
		layerIdName = StringTable->insert("Name");
		layerIndexName = StringTable->insert("Layer");

		explicitCellPropertiesInitialized = true;
	}
}

//------------------------------------------------------------------------------

TmxMapAsset::~TmxMapAsset()
{
	if (mParser)
	{
		delete mParser;
	}
}

//------------------------------------------------------------------------------

void TmxMapAsset::initPersistFields()
{
	// Call parent.
	Parent::initPersistFields();

	// Fields.
	addProtectedField("MapFile", TypeAssetLooseFilePath, Offset(mMapFile, TmxMapAsset), &setMapFile, &getMapFile, &defaultProtectedWriteFn, "");

}

//------------------------------------------------------------------------------

bool TmxMapAsset::onAdd()
{
	// Call Parent.
	if (!Parent::onAdd())
		return false;

	return true;
}

//------------------------------------------------------------------------------

void TmxMapAsset::onRemove()
{
	// Call Parent.
	Parent::onRemove();
}


//------------------------------------------------------------------------------

void TmxMapAsset::setMapFile( const char* pMapFile )
{
	// Sanity!
	AssertFatal( pMapFile != NULL, "Cannot use a NULL map file." );

	// Fetch image file.
	pMapFile = StringTable->insert( pMapFile );

	// Ignore no change,
	if ( pMapFile == mMapFile )
		return;

	// Update.
	mMapFile = getOwned() ? expandAssetFilePath( pMapFile ) : StringTable->insert( pMapFile );

	// Refresh the asset.
	refreshAsset();
}

//------------------------------------------------------------------------------

void TmxMapAsset::initializeAsset( void )
{
	// Call parent.
	Parent::initializeAsset();

	// Ensure the image-file is expanded.
	mMapFile = expandAssetFilePath( mMapFile );

	calculateMap();
}

//------------------------------------------------------------------------------

void TmxMapAsset::onAssetRefresh( void ) 
{
	// Ignore if not yet added to the sim.
	if ( !isProperlyAdded() )
		return;

	// Call parent.
	Parent::onAssetRefresh();

	calculateMap();
}	

//-----------------------------------------------------------------------------

void TmxMapAsset::onTamlPreWrite( void )
{
	// Call parent.
	Parent::onTamlPreWrite();

	// Ensure the image-file is collapsed.
	mMapFile = collapseAssetFilePath( mMapFile );
}

//-----------------------------------------------------------------------------

void TmxMapAsset::onTamlPostWrite( void )
{
	// Call parent.
	Parent::onTamlPostWrite();

	// Ensure the image-file is expanded.
	mMapFile = expandAssetFilePath( mMapFile );
}

//-----------------------------------------------------------------------------

void TmxMapAsset::onTamlCustomRead( const TamlCustomProperties& customProperties )
{
	// Debug Profiling.
	PROFILE_SCOPE(TmxMapAsset_OnTamlCustomRead);

	// Call parent.
	Parent::onTamlCustomRead( customProperties );

	// Find cell custom property
	const TamlCustomProperty* pLayerProperty = customProperties.findProperty( layerCustomPropertyName );

	if ( pLayerProperty != NULL )
	{
		for( TamlCustomProperty::const_iterator propertyAliasItr = pLayerProperty->begin(); propertyAliasItr != pLayerProperty->end(); ++propertyAliasItr )
		{
			// Fetch property alias.
			TamlPropertyAlias* pPropertyAlias = *propertyAliasItr;

			// Fetch alias name.
			StringTableEntry aliasName = pPropertyAlias->mAliasName;

			if (aliasName != layerAliasName)
			{
				// No, so warn.
				Con::warnf( "TmxMapAsset::onTamlCustomRead() - Encountered an unknown custom alias name of '%s'.  Only '%s' is valid.", aliasName, layerAliasName );
				continue;
			}


			S32 layerNumber = 0;
			StringTableEntry tmxLayerName = StringTable->EmptyString;

			// Iterate property fields.
			for ( TamlPropertyAlias::const_iterator propertyFieldItr = pPropertyAlias->begin(); propertyFieldItr != pPropertyAlias->end(); ++propertyFieldItr )
			{
				// Fetch property field.
				TamlPropertyField* pPropertyField = *propertyFieldItr;

				// Fetch property field name.
				StringTableEntry fieldName = pPropertyField->getFieldName();

				if (fieldName == layerIdName)
				{
					tmxLayerName = StringTable->insert( pPropertyField->getFieldValue() );
				}
				else if (fieldName == layerIndexName)
				{
					pPropertyField->getFieldValue( layerNumber );
				}
				else
				{
					// Unknown name so warn.
					Con::warnf( "TmxMapAsset::onTamlCustomRead() - Encountered an unknown custom field name of '%s'.", fieldName );
					continue;
				}
			}

			LayerOverride lo(tmxLayerName, layerNumber);

			mLayerOverrides.push_back(lo);

		}
	}
}
//-----------------------------------------------------------------------------

void TmxMapAsset::onTamlCustomWrite( TamlCustomProperties& customProperties )
{
	// Debug Profiling.
	PROFILE_SCOPE(TmxMapAsset_OnTamlCustomWrite);

	// Call parent.
	Parent::onTamlCustomWrite( customProperties );

	// Finish if nothing to write.
	if ( !mLayerOverrides.size() == 0 )
		return;

	// Add cell custom property.
	TamlCustomProperty* pLayerProperty = customProperties.addProperty( layerCustomPropertyName );

	// Iterate explicit frames.
	for( auto itr = mLayerOverrides.begin(); itr != mLayerOverrides.end(); ++itr )
	{
		// Fetch pixel area.
		auto overrideLayer = *itr;

		// Add cell alias.
		TamlPropertyAlias* pLayerAlias = pLayerProperty->addAlias( layerAliasName );

		// Add cell properties.
		pLayerAlias->addField( layerIdName, overrideLayer.LayerName );
		pLayerAlias->addField( layerIndexName, overrideLayer.SceneLayer );
	}

}

//----------------------------------------------------------------------------

void TmxMapAsset::calculateMap()
{
	if (mParser)
	{
		delete mParser;
		mParser = NULL;
	}

	mParser = new Tmx::Map();
	mParser->ParseFile( mMapFile );

	if (mParser->HasError())
	{
		// No, so warn.
		Con::warnf( "Map '%' could not be parsed: error code (%d) - %s.", getAssetId(), mParser->GetErrorCode(), mParser->GetErrorText().c_str() );
		delete mParser;
		mParser = NULL;
		return;
	}
}

bool TmxMapAsset::isAssetValid()
{
	return (mParser != NULL);
}

StringTableEntry TmxMapAsset::getOrientation()
{
	if (!isAssetValid()) return StringTable->EmptyString;

	switch(mParser->GetOrientation())
	{
	case Tmx::TMX_MO_ORTHOGONAL:
		{
			return StringTable->insert("ortho");
		}
	case Tmx::TMX_MO_ISOMETRIC:
		{
			return StringTable->insert("iso");
		}
	case Tmx::TMX_MO_STAGGERED:
		{
			return StringTable->insert("stag");
		}
	default:
		{
			return StringTable->EmptyString;
		}
	}
}

S32 TmxMapAsset::getLayerCount()
{
	if (!isAssetValid()) return 0;

	return mParser->GetNumLayers();
}

Tmx::Map* TmxMapAsset::getParser()
{
	if (!isAssetValid()) return NULL;

	return mParser;
}

S32 TmxMapAsset::getSceneLayer(const char* tmxLayerName)
{
	StringTableEntry layerName = StringTable->insert(tmxLayerName);

	auto itr = mLayerOverrides.begin();
	for(itr; itr != mLayerOverrides.end(); ++itr)
	{
		if ( (*itr).LayerName == layerName )
			return (*itr).SceneLayer;
	}

	return 0;
}

void TmxMapAsset::setSceneLayer(const char*tmxLayerName, S32 layerIdx)
{
	StringTableEntry layerName = StringTable->insert(tmxLayerName);
	auto itr = mLayerOverrides.begin();
	for(itr; itr != mLayerOverrides.end(); ++itr)
	{
		if ( (*itr).LayerName == layerName )
		{
			Con::warnf("Layer %s is already defined with an index of %d", layerName, (*itr).SceneLayer);
			return;
		}
	}

	LayerOverride lo(layerName, layerIdx);
	mLayerOverrides.push_back(lo);
}

StringTableEntry TmxMapAsset::getLayerOverrideName(int idx)
{
	if (idx >= mLayerOverrides.size())
	{
		Con::warnf("TmxMapAsset_getLayerOverrideName() - invalid idx %d", idx);
		return StringTable->EmptyString;
	}
	return mLayerOverrides.at(idx).LayerName;
}