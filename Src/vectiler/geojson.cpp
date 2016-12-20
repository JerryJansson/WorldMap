#include "Precompiled.h"
#include "geojson.h"
#include "jerry.h"
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
void GeoJson::extractLine(const rapidjson::Value& _in, Line& _out, const Tile& _tile)
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
        extractLine(*itr, _out.back(), _tile);
    }
}
//-----------------------------------------------------------------------------
void GeoJson::extractFeature(const ELayerType layerType, const rapidjson::Value& _in, Feature& _out, const Tile& _tile, const float defaultHeight)
{
    const rapidjson::Value& properties = _in["properties"];

	_out.height = defaultHeight;

    for (auto itr = properties.MemberBegin(); itr != properties.MemberEnd(); ++itr)
	{
        const auto& member = itr->name.GetString();
        const rapidjson::Value& prop = properties[member];
		
		//if (strcmp(member, "height") == 0)		{ _out.props.numericProps[member] = prop.GetDouble(); continue; }
		//if (strcmp(member, "min_height") == 0)	{ _out.props.numericProps[member] = prop.GetDouble(); continue; }
		//if (prop.IsNumber()) { //_out.props.numericProps[member] = prop.GetDouble(); }
		//else if (prop.IsString()) { //_out.props.stringProps[member] = prop.GetString(); }
		if (strcmp(member, "height") == 0)			_out.height		= prop.GetDouble();
        else if (strcmp(member, "min_height") == 0) _out.min_height = prop.GetDouble();
		else if (strcmp(member, "sort_rank") == 0)  _out.sort_rank	= prop.GetInt();
		else if (strcmp(member, "name") == 0)		_out.name		= prop.GetString();
		else if (strcmp(member, "id") == 0)			_out.id			= prop.GetInt64(); // Can be signed
		else if (layerType == eLayerRoads)
		{
			if (strcmp(member, "kind") == 0)
			{
				float w = 0.1f;
				const char* kind = prop.GetString();
				if (strcmp(kind, "highway") == 0)			w = 8;
				else if(strcmp(kind, "major_road") == 0)	w = 5;
				else if (strcmp(kind, "minor_road") == 0)	w = 3;
				else if (strcmp(kind, "rail") == 0)			w = 1;
				else if (strcmp(kind, "path") == 0)			w = 0.5f;
				else if (strcmp(kind, "ferry") == 0)		w = 2;
				else if (strcmp(kind, "piste") == 0)		w = 5;
				else if (strcmp(kind, "aerialway") == 0)	w = 6;
				else if (strcmp(kind, "aeroway") == 0)		w = 8;
				else if (strcmp(kind, "racetrack") == 0)	w = 3;
				else if (strcmp(kind, "portage_way") == 0)	w = 1;
				else
				{
					int abba = 10;
				}

				_out.road_width = w;
			}
		}
    }

	if (_out.min_height > _out.height)
	{
		int abba = 10;
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
        _out.lines.emplace_back();
        extractLine(coords, _out.lines.back(), _tile);
    } 
	else if (geometryType == "MultiLineString")
	{
        _out.geometryType = GeometryType::lines;
        for (auto lineCoords = coords.Begin(); lineCoords != coords.End(); ++lineCoords)
		{
            _out.lines.emplace_back();
            extractLine(*lineCoords, _out.lines.back(), _tile);
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
