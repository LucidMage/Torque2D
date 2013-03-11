#include "assets/assetManager.h"
#include "console/console.h"

#include "2d/sceneobject/TmxMapSprite.h"
#include "2d/sceneobject/TmxMapSprite_ScriptBinding.h"

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(TmxMapSprite);

//------------------------------------------------------------------------------

TmxMapSprite::TmxMapSprite() : mMapPixelToMeterFactor(0.03f),
	mLastTileAsset(StringTable->EmptyString),
	mLastTileImage(StringTable->EmptyString)
{
	// Reset local extents.
	mLocalExtents.SetZero();
	mLocalExtentsDirty = true;

	mTileSize.SetZero();
	mbIsIsoMap = false;

	mAutoSizing = true;
	setSleepingAllowed(true);
	setBodyType(b2_staticBody);
}

//------------------------------------------------------------------------------

TmxMapSprite::~TmxMapSprite()
{
	ClearMap();
}

void TmxMapSprite::initPersistFields()
{
	// Call parent.
	Parent::initPersistFields();

	addProtectedField("Map", TypeTmxMapAssetPtr, Offset(mMapAsset, TmxMapSprite), &setMap, &getMap, &writeMap, "");
	addProtectedField("MapToMeterFactor", TypeF32, Offset(mMapPixelToMeterFactor, TmxMapSprite), &setMapToMeterFactor, &getMapToMeterFactor, &writeMapToMeterFactor, "");
}

bool TmxMapSprite::onAdd()
{
	return (Parent::onAdd());

}

void TmxMapSprite::onRemove()
{
	Parent::onRemove();
}

void TmxMapSprite::OnRegisterScene(Scene* scene)
{
	Parent::OnRegisterScene(scene);

	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		scene->addToScene(*layerIdx);
	}

	auto collisionidx = mCollisionTiles.begin();
	for(collisionidx; collisionidx != mCollisionTiles.end(); ++collisionidx)
	{
		scene->addToScene(*collisionidx);
	}

	setLocalExtentsDirty();
	updateLocalExtents();
}

void TmxMapSprite::OnUnregisterScene( Scene* pScene )
{
	Parent::OnUnregisterScene(pScene);

	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		pScene->removeFromScene(*layerIdx);
	}

	auto collisionidx = mCollisionTiles.begin();
	for(collisionidx; collisionidx != mCollisionTiles.end(); ++collisionidx)
	{
		pScene->removeFromScene(*collisionidx);
	}
	
	setLocalExtentsDirty();

}

void TmxMapSprite::setPosition( const Vector2& position )
{
	auto posDiff = getPosition() - position;

	Parent::setPosition(position);
	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		(*layerIdx)->setPosition(position);
	}

	auto collisionidx = mCollisionTiles.begin();
	for(collisionidx; collisionidx != mCollisionTiles.end(); ++collisionidx)
	{
		SceneObject* obj = *collisionidx;
		obj->setPosition( obj->getPosition() + posDiff );
	}

	setLocalExtentsDirty();
	updateLocalExtents();

}

void TmxMapSprite::setAngle( const F32 radians )
{
	Parent::setAngle(radians);
	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		(*layerIdx)->setAngle(radians);
	}

	auto collisionidx = mCollisionTiles.begin();
	for(collisionidx; collisionidx != mCollisionTiles.end(); ++collisionidx)
	{
		(*collisionidx)->setAngle(radians);
	}

	setLocalExtentsDirty();
	updateLocalExtents();

}

//------------------------------------------------------------------------------

void TmxMapSprite::updateLocalExtents( void )
{
	// Debug Profiling.
	PROFILE_SCOPE(TmxMapSprite_UpdateLocalExtents);

	// Finish if the local extents are not dirty.
	if ( !mLocalExtentsDirty )
		return;

	// Flag as NOT dirty.
	mLocalExtentsDirty = false;

	// Do we have any sprites?
	if ( mLayers.size() == 0 )
	{
		// No, so reset local extents.
		mLocalExtents.setOne();

		return;
	}

	// Fetch first layer.
	auto layerItr = mLayers.begin();
	auto compSprite = *layerItr;

	Vector2 newExtent(0,0);

	// Combine with the rest of the sprites.
	for( ; layerItr != mLayers.end(); ++layerItr )
	{
		auto spSize = compSprite->getLocalExtents();
		newExtent.x = getMax(newExtent.x, spSize.x);
		newExtent.y = getMax(newExtent.y, spSize.y);
	}

	mLocalExtents = newExtent;
	setSize(mLocalExtents);
}


void TmxMapSprite::ClearMap()
{
	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		CompositeSprite* sprite = *layerIdx;
		sprite->deleteObject();
	}
	mLayers.clear();

	auto collisionidx = mCollisionTiles.begin();
	for(collisionidx; collisionidx != mCollisionTiles.end(); ++collisionidx)
	{
		auto collisonObject = *collisionidx;
		collisonObject->deleteObject();
	}
	mCollisionTiles.clear();

	mTileSize.setZero();
	mbIsIsoMap = false;
}


void TmxMapSprite::BuildMap()
{
	// Debug Profiling.
	PROFILE_SCOPE(TmxMapSprite_BuildMap);

	ClearMap();

	if (mMapAsset.isNull()) return;
	auto mapParser = mMapAsset->getParser();

	Tmx::MapOrientation orient = mapParser->GetOrientation();

	F32 tileWidth =  static_cast<F32>( mapParser->GetTileWidth() );
	F32 tileHeight = static_cast<F32>( mapParser->GetTileHeight() );
	F32 halfTileHeight = tileHeight * 0.5f;

	F32 width = (mapParser->GetWidth() * tileWidth);
	F32 height = (mapParser->GetHeight() * tileHeight);

	mTileSize.Set(tileWidth, tileHeight);
	F32 originY = 0;
	F32 originX = 0;

	if (orient == Tmx::TMX_MO_ISOMETRIC)
	{
		mbIsIsoMap = true;
		originY = height / 2 - halfTileHeight;
	}
	else
	{
		mbIsIsoMap = false;
		originX = -width/2;
		originY = height/2; 
	}


	Vector2 tileSize(tileWidth, tileHeight);
	Vector2 originSize(originX, originY);


	auto layerItr = mapParser->GetLayers().begin();
	for(layerItr; layerItr != mapParser->GetLayers().end(); ++layerItr)
	{
		Tmx::Layer* layer = *layerItr;

		//default to layer 0, unless a property is added to the layer that overrides it.
		int layerNumber = -1;
		if ( layer->GetProperties().HasProperty(TMX_MAP_LAYER_ID_PROP))
			layerNumber = layer->GetProperties().GetNumericProperty(TMX_MAP_LAYER_ID_PROP);

		auto assetLayerData = getLayerAssetData(  StringTable->insert(layer->GetName().c_str()) );

		auto compSprite = CreateLayer(assetLayerData, layerNumber, true, orient == Tmx::TMX_MO_ISOMETRIC);

		int xTiles = mapParser->GetWidth();
		int yTiles = mapParser->GetHeight();
		for(int x=0; x < xTiles; ++x)
		{
			for (int y=0; y < yTiles; ++y)
			{
				auto tile = layer->GetTile(x, y);
				if (tile.tilesetId == -1) continue; //no tile at this location

				auto tset = mapParser->GetTileset(tile.tilesetId);

				StringTableEntry assetName = GetTilesetAsset(tset);
				if (assetName == StringTable->EmptyString) continue;


				int localFrame = tile.id;
				F32 spriteHeight = static_cast<F32>( tset->GetTileHeight() );
				F32 spriteWidth = static_cast<F32>( tset->GetTileWidth() );

				F32 heightOffset = (spriteHeight - tileHeight) / 2.0f;
				F32 widthOffset = (spriteWidth - tileWidth) / 2.0f;

				Vector2 pos = TileToCoord( Vector2( static_cast<F32>(x),static_cast<F32>(y) ),
					tileSize,
					originSize,
					orient == Tmx::TMX_MO_ISOMETRIC
					);
				pos.add(Vector2(widthOffset, heightOffset));
				pos *= mMapPixelToMeterFactor;

				if (assetLayerData.mShouldRender)
				{
					auto bId = compSprite->addSprite( SpriteBatchItem::LogicalPosition( pos.scriptThis()) );
					compSprite->setSpriteImage(assetName, localFrame);
					compSprite->setSpriteSize( Vector2( spriteWidth * mMapPixelToMeterFactor, spriteHeight * mMapPixelToMeterFactor ) );

					compSprite->setSpriteFlipX(tile.flippedHorizontally);
					compSprite->setSpriteFlipY(tile.flippedVertically);
				}

				if (assetLayerData.mUseObjects)
				{
					//see if this tile has any defined objects.
					StringTableEntry tagName = StringTable->EmptyString;
					auto tileProps = tset->GetTile(tile.id);
					if (tileProps && tileProps->GetProperties().HasProperty(TMX_MAP_TILE_TAG_PROP))
					{
						tagName = StringTable->insert(tset->GetTile(tile.id)->GetProperties().GetLiteralProperty(TMX_MAP_TILE_TAG_PROP).c_str());
					}

					Vector<SceneObject*> objects;

					if (tagName != StringTable->EmptyString)
					{
						objects = mMapAsset->getTileObjectsByTag(tagName);
					}
					else
					{
						objects = mMapAsset->getTileObjects( tile.tilesetId + tile.id );
					}

					for(auto objItr = objects.begin(); objItr != objects.end(); ++objItr)
					{
						auto baseObject = *objItr;

						SceneObject* newObj = new SceneObject();
						baseObject->copyTo(newObj);

						newObj->setAwake(false);
						newObj->setSize( Vector2( spriteWidth * mMapPixelToMeterFactor, spriteHeight * mMapPixelToMeterFactor ) );

						Vector2 objPos(pos + this->getPosition());
						newObj->setPosition(objPos);
						newObj->setSceneLayer( assetLayerData.mSceneLayer );
						newObj->registerObject();
						mCollisionTiles.push_back(newObj);

						if (getScene() != NULL)
							getScene()->addToScene( newObj );

					}
				}
			}
		}

	}

	auto groupIdx = mapParser->GetObjectGroups().begin();
	for(groupIdx; groupIdx != mapParser->GetObjectGroups().end(); ++groupIdx)
	{
		auto groupLayer = *groupIdx;

		//default to layer 0, unless a property is added to the layer that overrides it.
		int layerNumber = 0;
		layerNumber = groupLayer->GetProperties().GetNumericProperty(TMX_MAP_LAYER_ID_PROP);

		auto assetLayerData = getLayerAssetData(  StringTable->insert(groupLayer->GetName().c_str()) );

		auto compSprite = CreateLayer(assetLayerData, layerNumber, false, orient == Tmx::TMX_MO_ISOMETRIC);

		auto objectIdx = groupLayer->GetObjects().begin();
		for(objectIdx; objectIdx != groupLayer->GetObjects().end(); ++objectIdx)
		{
			auto object = *objectIdx;

			auto gid = object->GetGid();

			auto tileSet = mapParser->FindTileset(gid);
			if (tileSet == NULL) continue;

			StringTableEntry assetName = GetTilesetAsset(tileSet);
			if (assetName == StringTable->EmptyString) continue;

			F32 objectWidth = static_cast<F32>( tileSet->GetTileWidth() );
			F32 objectHeight = static_cast<F32>( tileSet->GetTileHeight() );

			F32 heightOffset =(objectHeight - tileHeight) / 2;
			F32 widthOffset = (objectWidth - tileWidth) / 2;

			Vector2 vTile = CoordToTile( Vector2(static_cast<F32>( object->GetX() ), static_cast<F32>( object->GetY() ) ),
				tileSize,
				orient == Tmx::TMX_MO_ISOMETRIC
				);

			//TODO: I need to do more testing around object tile map positions. iso seem to be offset by -1,-1 tiles, while orth is 0,-1. 
			//Not sure if it's related to the object tile size compared to the map size, etc.
			if (orient == Tmx::TMX_MO_ISOMETRIC)
				vTile.sub(Vector2(1,1));
			else
				vTile.sub(Vector2(0,1));

			Vector2 pos = TileToCoord( vTile,
				tileSize,
				originSize,
				orient == Tmx::TMX_MO_ISOMETRIC
				);
			pos.add(Vector2(widthOffset, heightOffset));
			pos *= mMapPixelToMeterFactor;

			S32 frameNumber = gid - tileSet->GetFirstGid();

			auto bId = compSprite->addSprite( SpriteBatchItem::LogicalPosition( pos.scriptThis()) );
			compSprite->selectSpriteId(bId);
			compSprite->setSpriteImage(assetName, frameNumber);
			compSprite->setSpriteSize(Vector2( objectWidth * mMapPixelToMeterFactor, objectHeight * mMapPixelToMeterFactor));
			compSprite->setSpriteFlipX(false);
			compSprite->setSpriteFlipY(false);

		}
	}

	setLocalExtentsDirty();

}

Vector2 TmxMapSprite::CoordToTile(Vector2& pos, Vector2& tileSize, bool isIso)
{

	if (isIso)
	{
		Vector2 newPos(
			pos.x / tileSize.y,
			pos.y / tileSize.y
			);

		return newPos;
	}
	else
	{
		Vector2 newPos(
			pos.x / tileSize.x,
			pos.y / tileSize.y
			);

		return newPos;	}
}

Vector2 TmxMapSprite::TileToCoord(Vector2& pos, Vector2& tileSize, Vector2& offset, bool isIso)
{
	if (isIso)
	{
		Vector2 newPos(
			(pos.x - pos.y) * tileSize.y,
			offset.y - (pos.x + pos.y) * tileSize.y * 0.5f
			);

		return newPos;
	}
	else
	{
		Vector2 newPos(
			offset.x + (pos.x * tileSize.x),
			offset.y - (pos.y * tileSize.y)
			);

		return newPos;
	}
}

TmxMapAsset::LayerOverride TmxMapSprite::getLayerAssetData(StringTableEntry layerName)
{
	TmxMapAsset::LayerOverride layerOverride(layerName, 0, true, true);
	auto overrideIdx = mMapAsset->getLayerOverrides().begin();
	for(overrideIdx; overrideIdx != mMapAsset->getLayerOverrides().end(); ++overrideIdx)
	{
		auto chkOverride = *overrideIdx;

		if (chkOverride.mLayerName == layerOverride.mLayerName)
		{
			layerOverride = chkOverride;
			break;
		}
	}
	return layerOverride;
}

CompositeSprite* TmxMapSprite::CreateLayer(const TmxMapAsset::LayerOverride& layerOverride, int layerIndex, bool isTileLayer, bool isIso)
{

	if (!layerOverride.mShouldRender)
		return NULL;

	CompositeSprite* compSprite = new CompositeSprite();
	compSprite->registerObject();
	mLayers.push_back(compSprite);

	auto scene = this->getScene();
	if (scene)
		scene->addToScene(compSprite);


	compSprite->setBatchLayout( CompositeSprite::NO_LAYOUT );
	compSprite->setPosition(getPosition());
	compSprite->setAngle(getAngle());

	if (layerIndex == -1)
		compSprite->setSceneLayer(layerOverride.mSceneLayer);
	else
		compSprite->setSceneLayer(layerIndex);

	compSprite->setBatchIsolated(false);
	dynamic_cast<SpriteBatch*>(compSprite)->setBatchCulling( true );
	return compSprite;
}

const char* TmxMapSprite::getFileName(const char* path)
{
	auto pStr = dStrrchr(path, '\\');
	if (pStr == NULL)
		pStr = dStrrchr(path, '/');
	if (pStr == NULL)
		return NULL;

	pStr = pStr+1;

	auto pDot = dStrrchr(pStr, '.');
	int file_len = pDot - pStr;
	if (file_len >= 1024) return NULL;

	char buffer[1024];
	dStrncpy(buffer, pStr, file_len);
	buffer[file_len] = 0;
	return StringTable->insert(buffer);
}

StringTableEntry TmxMapSprite::GetTilesetAsset(const Tmx::Tileset* tileSet)
{
	StringTableEntry assetName = StringTable->insert( tileSet->GetProperties().GetLiteralProperty(TMX_MAP_TILESET_ASSETNAME_PROP).c_str() );
	if (assetName == StringTable->EmptyString)
	{
		//we fall back to using the image filename as an asset name query.
		//if that comes back with any hits, we use the first one.
		//If you don't want to name your asset the same as your art tile set name, 
		//you need to set a custom "AssetName" property on the tile set to override this.

		auto imageSource = tileSet->GetImage()->GetSource().c_str();
		StringTableEntry imageName = getFileName(imageSource);

		if (imageName == mLastTileImage)
		{
			//this is a quick memoize optimization to cut down on asset queries.
			//if we are requesting the same assetName from what we already found, we just return that.
			return mLastTileAsset;
		}

		mLastTileImage = imageName;

		AssetQuery query;
		S32 count = AssetDatabase.findAssetName(&query, imageName);
		if (count > 0)
		{
			assetName = query.first();
			mLastTileAsset = StringTable->insert( assetName );
		}
	}
	
	return assetName;
}

StringTableEntry TmxMapSprite::getTileProperty(StringTableEntry layerName, StringTableEntry propName, int x, int y)
{
	if (mMapAsset.isNull()) StringTable->EmptyString;;
	auto mapParser = mMapAsset->getParser();

	auto layerItr = mapParser->GetLayers().begin();
	for(layerItr; layerItr != mapParser->GetLayers().end(); ++layerItr)
	{
		Tmx::Layer* layer = *layerItr;

		auto tmxLayerName = StringTable->insert(layer->GetName().c_str());

		if (tmxLayerName == layerName)
		{
			auto tile = layer->GetTile(x, y);
			if (tile.tilesetId != -1)
			{
				auto tset = mapParser->GetTileset(tile.tilesetId);
				auto tileProps = tset->GetTile(tile.id);
				if (tileProps && tileProps->GetProperties().HasProperty(propName) )
				{
					auto rStr = StringTable->insert(tileProps->GetProperties().GetLiteralProperty(propName).c_str());
					return rStr;
				}
			}
		}
	}

	return StringTable->EmptyString;
}