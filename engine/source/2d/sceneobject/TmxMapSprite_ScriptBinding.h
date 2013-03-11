ConsoleMethod(TmxMapSprite, getMapToMeterFactor, F32, 2, 2, "() Returns the factor used to translate map pixels into scene meters\n"
	"")
{
	return object->getMapToMeterFactor();
}

ConsoleMethod(TmxMapSprite, setMapToMeterFactor, void, 3, 3, "(factor) Sets the factor used to translate map pixels into scene meters\n"
	"")
{
	F32 factor = dAtof(argv[2]);
	object->setMapToMeterFactor(factor);
}


ConsoleMethod(TmxMapSprite, getTileProperty, const char*, 6, 6, "(layerName, propertyName, x, y) Gets a property value for the specific x/y coord on the given layer.\n"
	"The property value or blank if one can not be found.")
{
	StringTableEntry lName = StringTable->insert(argv[2]);
	StringTableEntry pName = StringTable->insert(argv[3]);
	S32 x = dAtoi(argv[4]);
	S32 y = dAtoi(argv[5]);

	return object->getTileProperty(lName, pName, x, y);
}

ConsoleMethod(TmxMapSprite, WorldCoordToTile, const char*, 3,3,"Convert a world cooridnate into a local tile coordinate\n"
	"@Return a tile coordinate as string (x y)")
{
	Vector2 worldPoint(argv[2]);
	auto localPoint = object->getLocalPoint(worldPoint);

	auto tilePoint = object->CoordToTile(localPoint, object->getTileSize(), object->isIsoMap());
	return tilePoint.scriptThis();
}