#include "TmxMapAsset.h"


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

static StringTableEntry			layerCustomNodeName;
static StringTableEntry			layerNodeName;
static StringTableEntry			layerIdName;
static StringTableEntry			layerIndexName;
static StringTableEntry			layerRenderName;
static StringTableEntry			layerObjectName;

static StringTableEntry			tileCustomNodeName;
static StringTableEntry			tileNodeName;
static StringTableEntry			tileGIDName;
static StringTableEntry			tileTagName;


//------------------------------------------------------------------------------

TmxMapAsset::TmxMapAsset() :  mMapFile(StringTable->EmptyString),
	mParser(NULL)

{
	if (!explicitCellPropertiesInitialized)
	{
		layerCustomNodeName =		StringTable->insert("Layers");
		layerNodeName =				StringTable->insert("Layer");
		layerIdName =				StringTable->insert("Name");
		layerIndexName =			StringTable->insert("Layer");
		layerRenderName =			StringTable->insert("Render");
		layerObjectName =			StringTable->insert("useObjects");

		tileCustomNodeName =		StringTable->insert("Tiles");
		tileNodeName =				StringTable->insert("Tile");
		tileGIDName =				StringTable->insert("Gid");
		tileTagName =				StringTable->insert("Tag");

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

	mLayerOverrides.clear();

	auto itr = mTileObjects.begin();
	for(itr; itr != mTileObjects.end(); ++itr)
	{
		Vector<SceneObject*> objects = (*itr).value;

		auto soItr = objects.begin();
		for(soItr; soItr != objects.end(); ++soItr)
		{
			(*soItr)->unregisterObject();
		}
		objects.clear();
	}
	mTileObjects.clear();

	auto itrTag = mTileObjectsByTag.begin();
	for(itrTag; itrTag != mTileObjectsByTag.end(); ++itrTag)
	{
		Vector<SceneObject*> objects = (*itrTag).value;

		auto soItr = objects.begin();
		for(soItr; soItr != objects.end(); ++soItr)
		{
			(*soItr)->unregisterObject();
		}
		objects.clear();
	}
	mTileObjectsByTag.clear();
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

void TmxMapAsset::onTamlCustomRead( const TamlCustomNodes& customNodes )
{
	// Debug Profiling.
	PROFILE_SCOPE(TmxMapAsset_OnTamlCustomRead);

	// Call parent.
	Parent::onTamlCustomRead( customNodes );

	// Find layer custom property
	const TamlCustomNode* pLayerNode = customNodes.findNode( layerCustomNodeName );
	if ( pLayerNode != NULL )
	{
		// Fetch children layer nodes.
		const TamlCustomNodeVector& cellNodes = pLayerNode->getChildren();

		for( TamlCustomNodeVector::const_iterator nodeItr = cellNodes.begin(); nodeItr != cellNodes.end(); ++nodeItr )
		{
			// Fetch property alias.
			TamlCustomNode* pNode = *nodeItr;

			StringTableEntry nodeName = pNode->getNodeName();
			if (nodeName != layerNodeName)
			{
				// No, so warn.
				Con::warnf( "TmxMapAsset::onTamlCustomRead() - Encountered an unknown custom node name of '%s'.  Only '%s' is valid.", nodeName, layerNodeName );
				continue;
			}


			S32 layerNumber = 0;
			bool layerRender = true;
			bool useObjects = true;
			StringTableEntry tmxLayerName = StringTable->EmptyString;

			// Fetch fields.
			const TamlCustomFieldVector& fields = pNode->getFields();

			// Iterate property fields.
			for ( TamlCustomFieldVector::const_iterator fieldItr = fields.begin(); fieldItr != fields.end(); ++fieldItr )
			{
				// Fetch property field.
				TamlCustomField* pNodeField = *fieldItr;

				// Fetch property field name
				StringTableEntry fieldName = pNodeField->getFieldName();

				if (fieldName == layerIdName)
				{
					tmxLayerName = StringTable->insert( pNodeField->getFieldValue() );
				}
				else if (fieldName == layerIndexName)
				{
					pNodeField->getFieldValue( layerNumber );
				}
				else if (fieldName == layerRenderName)
				{
					pNodeField->getFieldValue(layerRender);
				}
				else if (fieldName == layerObjectName)
				{
					pNodeField->getFieldValue(useObjects);
				}
				else
				{
					// Unknown name so warn.
					Con::warnf( "TmxMapAsset::onTamlCustomRead() - Encountered an unknown custom field name of '%s'.", fieldName );
					continue;
				}
			}

			LayerOverride lo(tmxLayerName, layerNumber,layerRender, useObjects);

			mLayerOverrides.push_back(lo);

		}
	}

	//look for tile custom properties
	const TamlCustomNode* pTilesNode = customNodes.findNode( tileCustomNodeName );
	if ( pTilesNode != NULL )
	{
		// Fetch children tile nodes.
		const TamlCustomNodeVector& tileNodes = pTilesNode->getChildren();

		for( TamlCustomNodeVector::const_iterator nodeItr = tileNodes.begin(); nodeItr != tileNodes.end(); ++nodeItr )
		{
			// Fetch each node
			TamlCustomNode* pNode = *nodeItr;

			StringTableEntry nodeName = pNode->getNodeName();
			if (nodeName != tileNodeName)
			{
				// No, so warn.
				Con::warnf( "TmxMapAsset::onTamlCustomRead() - Encountered an unknown custom node name of '%s'.  Only '%s' is valid.", nodeName, tileNodeName );
				continue;
			}

			S32 gId = 0;
			StringTableEntry tagName = StringTable->EmptyString;
			Vector<SceneObject*> objects;

			// Fetch fields.
			const TamlCustomFieldVector& fields = pNode->getFields();

			// Iterate property fields.
			for ( TamlCustomFieldVector::const_iterator fieldItr = fields.begin(); fieldItr != fields.end(); ++fieldItr )
			{
				// Fetch property field.
				TamlCustomField* pNodeField = *fieldItr;

				// Fetch property field name
				StringTableEntry fieldName = pNodeField->getFieldName();

				if (fieldName == tileGIDName)
				{
					pNodeField->getFieldValue( gId );
				}
				else if (fieldName == tileTagName)
				{
					tagName =  StringTable->insert( pNodeField->getFieldValue() );
				}
				else
				{
					// Unknown name so warn.
					Con::warnf( "TmxMapAsset::onTamlCustomRead() - Encountered an unknown custom field name of '%s'.", fieldName );
					continue;
				}
			}

			if (gId ==0 && tagName == StringTable->EmptyString)
			{
				Con::warnf( "TmxMapAsset::onTamlCustomRead() - Tile node needs gId or Tag defined : '%s'.", nodeName );
				continue;
			}

			//check for any child SceneObject templates.
			const TamlCustomNodeVector& sceneNodes = pNode->getChildren();
			for( TamlCustomNodeVector::const_iterator nodeItr = sceneNodes.begin(); nodeItr != sceneNodes.end(); ++nodeItr )
			{
				// Fetch each node
				TamlCustomNode* pNode = *nodeItr;

				if ( pNode->isProxyObject() )
				{
					auto obj = pNode->getProxyObject<SceneObject>(true);
					objects.push_back(obj);
				}
			}


			if (tagName == StringTable->EmptyString)
				mTileObjects.insertUnique(gId, objects);
			else
				mTileObjectsByTag.insertUnique(tagName, objects);

		}
	}
}
//-----------------------------------------------------------------------------

void TmxMapAsset::onTamlCustomWrite( TamlCustomNodes& customNodes )
{
	// Debug Profiling.
	PROFILE_SCOPE(TmxMapAsset_OnTamlCustomWrite);

	// Call parent.
	Parent::onTamlCustomWrite( customNodes );

	// Finish if nothing to write.
	if ( !mLayerOverrides.size() == 0 )
		return;

	// Add cell custom property.
	TamlCustomNode* pNode = customNodes.addNode( layerCustomNodeName );

	for( auto itr = mLayerOverrides.begin(); itr != mLayerOverrides.end(); ++itr )
	{
		auto overrideLayer = *itr;

		// Add cell alias.
		TamlCustomNode* pSubNode = pNode->addNode( layerNodeName );

		// Add cell properties.
		pSubNode->addField( layerIdName, overrideLayer.mLayerName );
		pSubNode->addField( layerIndexName, overrideLayer.mSceneLayer );
		pSubNode->addField( layerRenderName, overrideLayer.mShouldRender );
		pSubNode->addField( layerObjectName, overrideLayer.mUseObjects );
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
		if ( (*itr).mLayerName == layerName )
			return (*itr).mSceneLayer;
	}

	return 0;
}

void TmxMapAsset::setSceneLayer(const char*tmxLayerName, S32 layerIdx, bool shouldRender, bool useObjects)
{
	StringTableEntry layerName = StringTable->insert(tmxLayerName);
	auto itr = mLayerOverrides.begin();
	for(itr; itr != mLayerOverrides.end(); ++itr)
	{
		if ( (*itr).mLayerName == layerName )
		{
			Con::warnf("Layer %s is already defined with an index of %d", layerName, (*itr).mSceneLayer);
			return;
		}
	}

	LayerOverride lo(layerName, layerIdx,shouldRender, useObjects);
	mLayerOverrides.push_back(lo);
}

StringTableEntry TmxMapAsset::getLayerOverrideName(int idx)
{
	if (idx >= mLayerOverrides.size())
	{
		Con::warnf("TmxMapAsset_getLayerOverrideName() - invalid idx %d", idx);
		return StringTable->EmptyString;
	}
	return mLayerOverrides.at(idx).mLayerName;
}

Vector<SceneObject*> TmxMapAsset::getTileObjects(S32 gId)
{
	auto objectsItr = mTileObjects.find(gId);

	if (objectsItr == mTileObjects.end())
	{
		return Vector<SceneObject*>();
	}
	else
	{
		return (*objectsItr).value;
	}
}

Vector<SceneObject*> TmxMapAsset::getTileObjectsByTag(StringTableEntry tag)
{
	auto objectsItr = mTileObjectsByTag.find(tag);

	if (objectsItr == mTileObjectsByTag.end())
	{
		return Vector<SceneObject*>();
	}
	else
	{
		return (*objectsItr).value;
	}
}
