#include "Precompiled.h"
#include "geojson.h"
#include "jerry.h"
Hash<CStr, EFeatureKind> gKindHash;
//-----------------------------------------------------------------------------
void CreateHash()
{
	gKindHash.Reserve(NUM_KINDS);
	
	gKindHash.Add("unknown", eKindUnknown);

	gKindHash.Add("highway", eKindHighway);
	gKindHash.Add("major_road", eKindMajorRoad);
	gKindHash.Add("minor_road", eKindMinorRoad);
	gKindHash.Add("rail", eKindRail);
	gKindHash.Add("path", eKindPath);
	gKindHash.Add("ferry", eKindFerry);
	gKindHash.Add("piste", eKindPiste);
	gKindHash.Add("aerialway", eKindAerialway);
	gKindHash.Add("aeroway", eKindAeroway);
	gKindHash.Add("racetrack", eKindRacetrack);
	gKindHash.Add("portage_way", eKindPortageway);

	// Landuse
	gKindHash.Add("aerodrome", eKindAerodrome);
	gKindHash.Add("attraction", eKindAttraction);
	gKindHash.Add("beach", eKindBeach);
	gKindHash.Add("bridge", eKindBridge);
	gKindHash.Add("forest", eKindForest);
	gKindHash.Add("golf_course", eKindGolfcourse);
	gKindHash.Add("grass", eKindGrass);
	gKindHash.Add("park", eKindPark);
	gKindHash.Add("pedestrian", eKindPedestrian);
	gKindHash.Add("railway", eKindRailway);
	gKindHash.Add("recreation_ground", eKindRecreationground);
	gKindHash.Add("residential", eKindResidential);

	for (int i = 0; i < NUM_KINDS; i++)
	{
		assert(gKindHash[i].val == i);
	}
}
//-----------------------------------------------------------------------------
bool GeoJson::extractPoint(const rapidjson::Value& _in, Point& _out, const Tile& _tile, Point* last)
{
    const Vec2d pos = LonLatToMeters(Vec2d(_in[0].GetDouble(), _in[1].GetDouble()));
	_out.x = (pos.x - _tile.tileOrigin.x);
	_out.y = (pos.y - _tile.tileOrigin.y);
	_out.z = 0;
    if (last && glm::length(_out - *last) < 1e-5f) {
        return false;
    }
    return true;
}
//-----------------------------------------------------------------------------
void GeoJson::extractLineString(const rapidjson::Value& _in, LineString& _out, const Tile& _tile)
{
    for (auto itr = _in.Begin(); itr != _in.End(); ++itr)
	{
        _out.emplace_back();
        if (_out.size() > 1)
		{
            if (!extractPoint(*itr, _out.back(), _tile, &_out[_out.size() - 2]))
                _out.pop_back();
        } 
		else
            extractPoint(*itr, _out.back(), _tile);
    }
}
//-----------------------------------------------------------------------------
void GeoJson::extractPoly(const rapidjson::Value& _in, Polygon2& _out, const Tile& _tile)
{
    for (auto itr = _in.Begin(); itr != _in.End(); ++itr)
	{
        _out.emplace_back();
        extractLineString(*itr, _out.back(), _tile);
    }
}
//-----------------------------------------------------------------------------
void GeoJson::extractFeature(const ELayerType layerType, const rapidjson::Value& _in, Feature& _out, const Tile& _tile, const float defaultHeight)
{
	static bool hashCreated = false;
	if (!hashCreated)
	{
		hashCreated = true;
		CreateHash();
	}

    const rapidjson::Value& properties = _in["properties"];

	_out.height = defaultHeight;

    for (auto itr = properties.MemberBegin(); itr != properties.MemberEnd(); ++itr)
	{
        const auto& member = itr->name.GetString();
        const rapidjson::Value& prop = properties[member];
		
		if (strcmp(member, "height") == 0)			_out.height		= prop.GetDouble();
        else if (strcmp(member, "min_height") == 0) _out.min_height = prop.GetDouble();
		else if (strcmp(member, "sort_rank") == 0)  _out.sort_rank	= prop.GetInt();
		else if (strcmp(member, "name") == 0)		_out.name		= prop.GetString();
		else if (strcmp(member, "id") == 0)			_out.id			= prop.GetInt64(); // Can be signed
		else if (strcmp(member, "kind") == 0)		_out.kind       = gKindHash.Get(prop.GetString());
    }

	if (_out.min_height > _out.height)
	{
		int abba = 10;
	}

	if (layerType == eLayerRoads)
	{
		switch (_out.kind)
		{
		case eKindHighway: _out.road_width = 8; break;
		case eKindMajorRoad: _out.road_width = 5; break;
		case eKindMinorRoad: _out.road_width = 3; break;
		case eKindRail: _out.road_width = 1; break;
		case eKindPath: _out.road_width = 0.5f; break;
		case eKindFerry: _out.road_width = 1; break;
		case eKindPiste: _out.road_width = 5; break;
		case eKindAerialway: _out.road_width = 6; break;
		case eKindAeroway: _out.road_width = 8; break;
		case eKindRacetrack: _out.road_width = 3; break;
		case eKindPortageway: _out.road_width = 1; break;
		default: _out.road_width = 0.1f; break;
		}
	}


    // Copy geometry into tile data
    const rapidjson::Value& geometry = _in["geometry"];
    const rapidjson::Value& coords = geometry["coordinates"];
    const std::string& geometryType = geometry["type"].GetString();

    if (geometryType == "Point")
	{
        _out.geometryType = GeometryType::points;
        _out.points.emplace_back();
        if (!extractPoint(coords, _out.points.back(), _tile)) { _out.points.pop_back(); }
    }
	else if (geometryType == "MultiPoint")
	{
        _out.geometryType= GeometryType::points;
        for (auto pointCoords = coords.Begin(); pointCoords != coords.End(); ++pointCoords) {
            if (!extractPoint(coords, _out.points.back(), _tile)) { _out.points.pop_back(); }
        }
    } 
	else if (geometryType == "LineString")
	{
        _out.geometryType = GeometryType::lines;
        _out.lineStrings.emplace_back();
        extractLineString(coords, _out.lineStrings.back(), _tile);
    } 
	else if (geometryType == "MultiLineString")
	{
        _out.geometryType = GeometryType::lines;
        for (auto lineCoords = coords.Begin(); lineCoords != coords.End(); ++lineCoords)
		{
            _out.lineStrings.emplace_back();
            extractLineString(*lineCoords, _out.lineStrings.back(), _tile);
        }
    } 
	else if (geometryType == "Polygon")
	{
        _out.geometryType = GeometryType::polygons;
        _out.polygons.emplace_back();
        extractPoly(coords, _out.polygons.back(), _tile);
    }
	else if (geometryType == "MultiPolygon")
	{
        _out.geometryType = GeometryType::polygons;
        for (auto polyCoords = coords.Begin(); polyCoords != coords.End(); ++polyCoords)
		{
            _out.polygons.emplace_back();
            extractPoly(*polyCoords, _out.polygons.back(), _tile);
        }
    }
}
//-----------------------------------------------------------------------------
void GeoJson::extractLayer(const rapidjson::Value& _in, Layer& _out, const Tile& _tile)
{
    const auto& featureIter = _in.FindMember("features");

    if (featureIter == _in.MemberEnd())
	{
        LOG("ERROR: GeoJSON missing 'features' member\n");
        return;
    }

	// Default values
	float defaultHeight = _out.layerType == eLayerBuildings ? 9.0f : 0.0f;
	
    const auto& features = featureIter->value;
    for (auto featureJson = features.Begin(); featureJson != features.End(); ++featureJson) 
	{
        _out.features.emplace_back();
        extractFeature(_out.layerType, *featureJson, _out.features.back(), _tile, defaultHeight);
    }
}
