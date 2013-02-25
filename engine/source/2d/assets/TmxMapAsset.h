#ifndef _TMXMAP_ASSET_H_
#define _TMXMAP_ASSET_H_

#ifndef _ASSET_PTR_H_
#include "assets/assetPtr.h"
#endif

#ifndef _VECTOR_H_
#include "collection/vector.h"
#endif

#include <Tmx.h>

//-----------------------------------------------------------------------------

DefineConsoleType( TypeTmxMapAssetPtr )

//-----------------------------------------------------------------------------

class TmxMapAsset : public AssetBase
{

////T2D SIM/CONSOLE setup////////////////////
private:
	typedef AssetBase Parent;

public:
	struct LayerOverride
	{
		StringTableEntry	LayerName;
		S32					SceneLayer;

	public: LayerOverride(StringTableEntry lName, S32 layerIdx)
			{
				LayerName = lName;
				SceneLayer = layerIdx;
			}
	};

public: 

	TmxMapAsset();
	virtual ~TmxMapAsset();

	/// Core.
	static void initPersistFields();
	virtual bool onAdd();
	virtual void onRemove();
	
	/// Declare Console Object.
	DECLARE_CONOBJECT(TmxMapAsset);
//////////////////////////////////////////////


private:

	/// Configuration.
	StringTableEntry            mMapFile;

	Vector<LayerOverride>		mLayerOverrides;

public:

	void                    setMapFile( const char* pMapFile );
	inline StringTableEntry getMapFile( void ) const                      { return mMapFile; };


	StringTableEntry getOrientation();
	S32				 getLayerCount();
	const Vector<LayerOverride>& getLayerOverrides(){return mLayerOverrides;}
	Tmx::Map*		 getParser();

	S32				getSceneLayer(const char* tmxLayerName);
	void			setSceneLayer(const char*tmxLayerName, S32 layerIdx);
	S32				getLayerOverrideCount(){return mLayerOverrides.size();}
	StringTableEntry getLayerOverrideName(int idx);
private:

	Tmx::Map*					mParser;

	void calculateMap( void );
	virtual bool isAssetValid();

protected:
	virtual void initializeAsset( void );
	virtual void onAssetRefresh( void );

	/// Taml callbacks.
	virtual void onTamlPreWrite( void );
	virtual void onTamlPostWrite( void );
	virtual void onTamlCustomWrite( TamlCustomProperties& customProperties );
	virtual void onTamlCustomRead( const TamlCustomProperties& customProperties );


protected:

	static bool setMapFile( void* obj, const char* data )                 { static_cast<TmxMapAsset*>(obj)->setMapFile(data); return false; }
	static const char* getMapFile(void* obj, const char* data)            { return static_cast<TmxMapAsset*>(obj)->getMapFile(); }

};

#endif //_TMXMAP_ASSET_H_