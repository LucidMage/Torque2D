#include "TmxMapSprite.h"

#include "assets/assetManager.h"

//-----------------------------------------------------------------------------

IMPLEMENT_CONOBJECT(TmxMapSprite);

//------------------------------------------------------------------------------

TmxMapSprite::TmxMapSprite() : mMapPixelToMeterFactor(0.03),
	mLastTileAsset(StringTable->EmptyString),
	mLastTileImage(StringTable->EmptyString)
{
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

}

void TmxMapSprite::OnUnregisterScene( Scene* pScene )
{
	Parent::OnUnregisterScene(pScene);

	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		pScene->removeFromScene(*layerIdx);
	}
}

void TmxMapSprite::setPosition( const Vector2& position )
{
	Parent::setPosition(position);
	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		(*layerIdx)->setPosition(position);
	}
}

void TmxMapSprite::setAngle( const F32 radians )
{
	Parent::setAngle(radians);
	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		(*layerIdx)->setAngle(radians);
	}
}

void TmxMapSprite::ClearMap()
{
	auto layerIdx = mLayers.begin();
	for(layerIdx; layerIdx != mLayers.end(); ++layerIdx)
	{
		CompositeSprite* sprite = *layerIdx;
		delete sprite;
	}
	mLayers.clear();
}


void TmxMapSprite::BuildMap()
{
	// Debug Profiling.
	PROFILE_SCOPE(TmxMapSprite_BuildMap);


	ClearMap();
	auto mapParser = mMapAsset->getParser();

	Tmx::MapOrientation orient = mapParser->GetOrientation();

	F32 tileWidth = mapParser->GetTileWidth();
	F32 tileHeight =mapParser->GetTileHeight();
	F32 halfTileHeight = tileHeight * 0.5;

	F32 width = (mapParser->GetWidth() * tileWidth);
	F32 height = (mapParser->GetHeight() * tileHeight);

	F32 originY = 0;
	F32 originX = 0;

	if (orient == Tmx::TMX_MO_ISOMETRIC)
	{
		originY = height / 2 - halfTileHeight;
	}
	else
	{
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
		int layerNumber = 0;
		layerNumber = layer->GetProperties().GetNumericProperty(TMX_MAP_LAYER_ID_PROP);

		auto compSprite = CreateLayer(layerNumber, orient == Tmx::TMX_MO_ISOMETRIC);

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
				F32 spriteHeight = tset->GetTileHeight();
				F32 spriteWidth = tset->GetTileWidth();

				F32 heightOffset = (spriteHeight - tileHeight) / 2;
				F32 widthOffset = (spriteWidth - tileWidth) / 2;

				Vector2 pos = TileToCoord( Vector2(x,y),
					tileSize,
					originSize,
					orient == Tmx::TMX_MO_ISOMETRIC
					);
				pos.add(Vector2(widthOffset, heightOffset));
				pos *= mMapPixelToMeterFactor;

				auto bId = compSprite->addSprite( SpriteBatchItem::LogicalPosition( pos.scriptThis()) );
				compSprite->selectSpriteId(bId);
				compSprite->setSpriteImage(assetName, localFrame);
				compSprite->setSpriteSize( Vector2( spriteWidth * mMapPixelToMeterFactor, spriteHeight * mMapPixelToMeterFactor ) );

				compSprite->setSpriteFlipX(tile.flippedHorizontally);
				compSprite->setSpriteFlipY(tile.flippedVertically);

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
		auto compSprite = CreateLayer(layerNumber, orient == Tmx::TMX_MO_ISOMETRIC);

		auto objectIdx = groupLayer->GetObjects().begin();
		for(objectIdx; objectIdx != groupLayer->GetObjects().end(); ++objectIdx)
		{
			auto object = *objectIdx;

			auto gid = object->GetGid();

			auto tileSet = mapParser->FindTileset(gid);
			if (tileSet == NULL) continue;

			StringTableEntry assetName = GetTilesetAsset(tileSet);
			if (assetName == StringTable->EmptyString) continue;

			F32 objectWidth = tileSet->GetTileWidth();
			F32 objectHeight = tileSet->GetTileHeight();

			F32 heightOffset =(objectHeight - tileHeight) / 2;
			F32 widthOffset = (objectWidth - tileWidth) / 2;

			Vector2 vTile = CoordToTile( Vector2(object->GetX(), object->GetY()),
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
			offset.y - (pos.x + pos.y) * tileSize.y * 0.5
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

CompositeSprite* TmxMapSprite::CreateLayer(int layerIndex, bool isIso)
{
	CompositeSprite* compSprite = new CompositeSprite();
	mLayers.push_back(compSprite);

	auto scene = this->getScene();
	if (scene)
		scene->addToScene(compSprite);


	compSprite->setBatchLayout( CompositeSprite::NO_LAYOUT );
	compSprite->setPosition(getPosition());
	compSprite->setAngle(getAngle());
	compSprite->setSceneLayer(layerIndex);
	compSprite->setBatchSortMode(SceneRenderQueue::RENDER_SORT_ZAXIS);
	compSprite->setBatchIsolated(true);
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