
ConsoleMethod(TmxMapAsset, setMapFile, void, 3, 3,     "(MapFile) Sets the map file (tmx file).\n"
	"@return No return value.")
{
	object->setMapFile( argv[2] );
}

//-----------------------------------------------------------------------------

ConsoleMethod(TmxMapAsset, getMapFile, const char*, 2, 2,  "() Gets the map file.\n"
	"@return Returns the tmx map file.")
{
	return object->getMapFile();
}

ConsoleMethod(TmxMapAsset, getOrientation, const char*, 2, 2,  "() Gets the map orientation.\n"
	"@return Returns the tmx map orientation layout.")
{
	return object->getOrientation();
}

ConsoleMethod(TmxMapAsset, getLayerCount, S32, 2, 2,  "() Gets the number of tile layers.\n"
	"@return Returns the numer of tile layers in the map.")
{
	return object->getLayerCount();
}

ConsoleMethod(TmxMapAsset, getLayerOverrideCount, S32, 2, 2,  "() Gets the number of layer override entries.\n"
	"@return Returns the number of layer override entries.")
{
	return object->getLayerOverrideCount();
}

ConsoleMethod(TmxMapAsset, getLayerOverrideName, const char*, 3, 3,  "(index) Gets the TMX layer name at (index).\n"
	"@return Returns the TMX layer name at the specified index.")
{
	return object->getLayerOverrideName( dAtoi(argv[2]) );
}

ConsoleMethod(TmxMapAsset, getSceneLayer, S32, 3, 3,  "(TMX_layer_name) Gets the overridden scene render layer for (TMX_layer_name).\n"
	"@return Returns  the overridden scene render layer defined for the given layer name.")
{
	return object->getSceneLayer( (argv[2]) );
}

ConsoleMethod(TmxMapAsset, setSceneLayer, void, 6, 6,  "(TMX_layer_name, scene_layer_number, shouldRender, useObjects) Sets the overridden scene render layer for (TMX_layer_name).\n"
	"")
{
	object->setSceneLayer( (argv[2]), dAtoi(argv[3]), dAtob(argv[4]), dAtob(argv[5]) );
}