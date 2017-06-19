#include "Precompiled.h"
#include "geojson.h"
//-----------------------------------------------------------------------------
static inline void extractP(const rapidjson::Value& _in, Point& p, const Tile& _tile)
{
	const Vec2d pos = LonLatToMeters(Vec2d(_in[0].GetDouble(), _in[1].GetDouble()));
	p.x = (pos.x - _tile.tileOrigin.x);
	p.y = (pos.y - _tile.tileOrigin.y);
	p.z = 0;
}//-----------------------------------------------------------------------------
static inline bool extractPoint(const rapidjson::Value& _in, Point& p, const Tile& _tile, Point* last)
{
	const Vec2d pos = LonLatToMeters(Vec2d(_in[0].GetDouble(), _in[1].GetDouble()));
	p.x = (pos.x - _tile.tileOrigin.x);
	p.y = (pos.y - _tile.tileOrigin.y);
	p.z = 0;
	//return myLength(p - *last) >= 1e-5f ? true : false;
	return sqLength(p - *last) >= 1e-10f ? true : false;
}
//-----------------------------------------------------------------------------
static inline int extractLineString(const rapidjson::Value& arr, LineString& l, const Tile& _tile)
{
	const int count = arr.Size();
	assert(count >= 2);
	l.reserve(count);
	l.emplace_back();
	extractP(arr[0], l.back(), _tile);

	for (int i=1; i<count; i++)
	{
		l.emplace_back();
		if (!extractPoint(arr[i], l.back(), _tile, &l[l.size() - 2]))
			l.pop_back();
	}

	return l.size();
}
//-----------------------------------------------------------------------------
// A poly is 1 or more linestrings
// First linestring can be poly, second linestring can be a hole
//-----------------------------------------------------------------------------
bool extractPoly(const rapidjson::Value& arr, Polygon2& poly, const Tile& _tile)
{
	const int count = arr.Size();
	poly.reserve(count);
	for(int i=0; i<count; i++)
	{
		poly.emplace_back();
		if (extractLineString(arr[i], poly.back(), _tile) < 3)	// Need at least 3 points in linestring to form a polygon
		{
			poly.pop_back();
			gLogger.Warning("GeoJson invalid polygon");
		}
	}

	return poly.size() == count;
}
//-----------------------------------------------------------------------------
bool SkipFeature(const ELayerType layerType, const Feature& f)
{
	// Skip other geometry than lines & polygons
	if ((f.geometryType == GeometryType::unknown || f.geometryType == GeometryType::points))
		return true;

	// Skip certain features drawn as lines
	if (f.geometryType == GeometryType::lines)
	{
		if (layerType == eLayerTransit)
		{
			if (f.kind == eKind_platform || f.kind == eKind_tram)
				return true;
		}
		else if (layerType == eLayerLanduse)
		{
			if (f.kind == eKind_city_wall)
				return true;
		}
	}

	// Skip water boundaries
	if (layerType == eLayerWater && f.boundary)
		return true;

	// Settings says skip this feature
	const MapGeom* g = gGeomHash.GetValuePtr((layerType << 16 | f.kind));
	if (g && g->skip)
		return true;

	return false;
}
//-----------------------------------------------------------------------------
bool GeoJson::extractFeature(const ELayerType layerType, const rapidjson::Value& _in, Feature& f, const Tile& _tile, const float defaultHeight)
{
	const rapidjson::Value& properties = _in["properties"];

	int c = properties.MemberCount();

	f.height = defaultHeight;

    for (auto itr = properties.MemberBegin(); itr != properties.MemberEnd(); ++itr)
	{
        const auto& member = itr->name.GetString();
        const rapidjson::Value& prop = properties[member];
		
		if (strcmp(member, "height") == 0)			f.height		= prop.GetDouble();
        else if (strcmp(member, "min_height") == 0) f.min_height	= prop.GetDouble();
		else if (strcmp(member, "sort_rank") == 0)  f.sort_rank		= prop.GetInt();
		else if (strcmp(member, "name") == 0)		f.name			= prop.GetString();
		else if (strcmp(member, "id") == 0)			f.id			= prop.GetInt64(); // Can be signed
		else if (strcmp(member, "kind") == 0)
		{
			const CStr tmpstr = prop.GetString();
			f.kind = gKindHash.Get(tmpstr);
			if (f.kind == eKind_unknown)
			{
				LOG("Ukn: %s - %s\n", layerNames[layerType], tmpstr.Str());
			}
		}
		else if (strcmp(member, "boundary") == 0)	f.boundary		= prop.GetBool();
    }

	if (f.min_height > f.height)
	{
		int abba = 10;
	}

	// Get feature's geometry type
	const rapidjson::Value& geometry = _in["geometry"];
	const rapidjson::Value& coords = geometry["coordinates"];
	const std::string& geomType = geometry["type"].GetString();

	if (geomType == "Point")				f.geometryType = GeometryType::points;
	else if (geomType == "MultiPoint")		f.geometryType = GeometryType::points;
	else if (geomType == "LineString")		f.geometryType = GeometryType::lines;
	else if (geomType == "MultiLineString")	f.geometryType = GeometryType::lines;
	else if (geomType == "Polygon")			f.geometryType = GeometryType::polygons;
	else if (geomType == "MultiPolygon")	f.geometryType = GeometryType::polygons;
		
	// Skip certain features we are not interested in
	if (SkipFeature(layerType, f))
	{
		//LOG("Skipped feature (%I64d)\n", f.id);
		return false;
	}

	// Copy geometry into tile data
    if (geomType == "Point")
	{
        f.points.emplace_back();
        extractP(coords, f.points.back(), _tile);
    }
	else if (geomType == "MultiPoint")
	{
		gLogger.TellUser(LOG_NOTIFY, "MultiPoint. Not sure if this might be buggy implementation?");
		for (auto pointCoords = coords.Begin(); pointCoords != coords.End(); ++pointCoords)
			extractP(coords, f.points.back(), _tile);
    } 
	else if (geomType == "LineString")
	{
        f.lineStrings.emplace_back();
		if (extractLineString(coords, f.lineStrings.back(), _tile) < 2)
		{
			gLogger.Warning("GeoJson invalid geometry");
			return false;
		}
    } 
	else if (geomType == "MultiLineString")
	{
        for (auto lineCoords = coords.Begin(); lineCoords != coords.End(); ++lineCoords)
		{
            f.lineStrings.emplace_back();
			if (extractLineString(*lineCoords, f.lineStrings.back(), _tile) < 2)
			{
				f.lineStrings.pop_back();
				gLogger.Warning("GeoJson invalid geometry");
			}
        }

		if (f.lineStrings.size() == 0)
		{
			gLogger.Warning("GeoJson invalid geometry");
			return false;
		}

    } 
	else if (geomType == "Polygon")
	{
        f.polygons.emplace_back();
        extractPoly(coords, f.polygons.back(), _tile);
    }
	else if (geomType == "MultiPolygon")
	{
        for (auto polyCoords = coords.Begin(); polyCoords != coords.End(); ++polyCoords)
		{
            f.polygons.emplace_back();
            extractPoly(*polyCoords, f.polygons.back(), _tile);
        }
    }

	return true;
}
// Unoptimized
// Parsed json in 81.7ms. Built GeoJson structures in 74.0ms
// Parsed json in 80.0ms.Built GeoJson structures in 76.2ms
// Parsed json in 79.2ms.Built GeoJson structures in 75.2ms
// Parsed json in 2.1ms.Built GeoJson structures in 2.0ms
// Parsed json in 7.0ms.Built GeoJson structures in 6.2ms
// Parsed json in 2.0ms.Built GeoJson structures in 3.2ms
// Reserve layers
// Parsed json in 77.0ms. Built GeoJson structures in 72.5ms
// Parsed json in 78.8ms.Built GeoJson structures in 75.6ms
// Parsed json in 64.5ms.Built GeoJson structures in 75.4ms
// Parsed json in 7.3ms.Built GeoJson structures in 8.0ms
// Parsed json in 2.8ms. Built GeoJson structures in 2.1ms
// Parsed json in 2.0ms. Built GeoJson structures in 1.8ms
// Reserve features
// Parsed json in 82.7ms. Built GeoJson structures in 53.4ms
// Parsed json in 74.3ms.Built GeoJson structures in 51.0ms
// Parsed json in 67.8ms.Built GeoJson structures in 48.7ms
// Parsed json in 2.0ms. Built GeoJson structures in 1.3ms
// Parsed json in 7.2ms. Built GeoJson structures in 5.9ms
// Parsed json in 6.9ms. Built GeoJson structures in 4.7ms
// ExtractP
// Parsed json in 73.4ms.Built GeoJson structures in 43.5ms
// Parsed json in 65.8ms.Built GeoJson structures in 42.5ms
// Parsed json in 70.4ms.Built GeoJson structures in 47.1ms
// Parsed json in 1.9ms.Built GeoJson structures in 1.2ms
// Parsed json in 2.1ms.Built GeoJson structures in 1.1ms
// Parsed json in 7.3ms.Built GeoJson structures in 3.2ms


// Laptop (Reserve features)
// Parsed json in 156.4ms.Built GeoJson structures in 76.6ms
// Parsed json in 138.4ms.Built GeoJson structures in 66.8ms
// Parsed json in 116.3ms.Built GeoJson structures in 70.7ms
// ExtractP
// Parsed json in 88.6ms. Built GeoJson structures in 64.1ms
// Parsed json in 104.3ms.Built GeoJson structures in 56.5ms
// Parsed json in 81.8ms.Built GeoJson structures in 55.6ms
//-----------------------------------------------------------------------------
void GeoJson::extractLayer(const rapidjson::Value& _in, Layer& layer, const Tile& _tile)
{
    const auto& featureIter = _in.FindMember("features");

    if (featureIter == _in.MemberEnd())
	{
        LOG("ERROR: GeoJSON missing 'features' member\n");
		return;
    }

	// Default values
	float defaultHeight = layer.layerType == eLayerBuildings ? 8.0f : 0.0f;
	
    const auto& features = featureIter->value;

	const int featureCount = features.Size();
	layer.features.EnsureCapacity(features.Size());
	for (auto featureJson = features.Begin(); featureJson != features.End(); ++featureJson) 
	{
		Feature& f = layer.features.AddEmpty();
		if (!extractFeature(layer.layerType, *featureJson, f, _tile, defaultHeight))
			layer.features.RemoveLast();
    }
}
