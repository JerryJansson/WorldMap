#include "Precompiled.h"
#include "geojson.h"
Hash<CStr, EFeatureKind> gKindHash;
//-----------------------------------------------------------------------------
void CreateHash()
{
	gKindHash.Reserve(NUM_KINDS);

#define ADDHASH(KIND) gKindHash.Add(#KIND, eKind_##KIND)
	ADDHASH(unknown);

	// Buildings
	ADDHASH(building);			ADDHASH(building_part);		ADDHASH(address);
	// Roads
	ADDHASH(aerialway);			ADDHASH(aeroway);			ADDHASH(ferry);
	ADDHASH(highway);			ADDHASH(major_road);		ADDHASH(minor_road);
	ADDHASH(path);				ADDHASH(piste);				ADDHASH(racetrack);
	ADDHASH(rail);				//ADDHASH(portage_way);
	// Landuse
	ADDHASH(aerodrome);			ADDHASH(allotments);		ADDHASH(amusement_ride);
	ADDHASH(animal);			ADDHASH(apron);				ADDHASH(aquarium);
	ADDHASH(artwork);			ADDHASH(attraction);		ADDHASH(aviary);
	ADDHASH(battlefield);		ADDHASH(beach);				ADDHASH(breakwater);
	ADDHASH(bridge);			ADDHASH(camp_site);			ADDHASH(caravan_site);
	ADDHASH(carousel);			ADDHASH(cemetery);			ADDHASH(cinema);
	ADDHASH(city_wall);			ADDHASH(college);			ADDHASH(commercial);
	ADDHASH(common);			ADDHASH(cutline);			ADDHASH(dam);
	ADDHASH(dike);				ADDHASH(dog_park);			ADDHASH(enclosure);
	ADDHASH(farm);				ADDHASH(farmland);			ADDHASH(farmyard);
	ADDHASH(fence);				ADDHASH(footway);			ADDHASH(forest);
	ADDHASH(fort);				ADDHASH(fuel);				ADDHASH(garden);
	ADDHASH(gate);				ADDHASH(generator);			ADDHASH(glacier);
	ADDHASH(golf_course);		ADDHASH(grass);				ADDHASH(grave_yard);
	ADDHASH(groyne);			ADDHASH(hanami);			ADDHASH(hospital);
	ADDHASH(industrial);		ADDHASH(land);				ADDHASH(library);
	ADDHASH(maze);				ADDHASH(meadow);			ADDHASH(military);
	ADDHASH(national_park);		ADDHASH(nature_reserve);	ADDHASH(natural_forest);
	ADDHASH(natural_park);		ADDHASH(natural_wood);		ADDHASH(park);
	ADDHASH(parking);			ADDHASH(pedestrian);		ADDHASH(petting_zoo);
	ADDHASH(picnic_site);		ADDHASH(pier);				ADDHASH(pitch);
	ADDHASH(place_of_worship);	ADDHASH(plant);				ADDHASH(playground);
	ADDHASH(prison);			ADDHASH(protected_area);	ADDHASH(quarry);
	ADDHASH(railway);			ADDHASH(recreation_ground);	ADDHASH(recreation_track);
	ADDHASH(residential);		ADDHASH(resort);			ADDHASH(rest_area);
	ADDHASH(retail);			ADDHASH(retaining_wall);	ADDHASH(rock);
	ADDHASH(roller_coaster);	ADDHASH(runway);			ADDHASH(rural);
	ADDHASH(school);			ADDHASH(scree);				ADDHASH(scrub);
	ADDHASH(service_area);		ADDHASH(snow_fence);		ADDHASH(sports_centre);
	ADDHASH(stadium);			ADDHASH(stone);				ADDHASH(substation);
	ADDHASH(summer_toboggan);	ADDHASH(taxiway);			ADDHASH(theatre);
	ADDHASH(theme_park);		ADDHASH(tower);				ADDHASH(trail_riding_station);
	ADDHASH(university);		ADDHASH(urban_area);		ADDHASH(urban);
	ADDHASH(village_green);		ADDHASH(wastewater_plant);	ADDHASH(water_park);
	ADDHASH(water_slide);		ADDHASH(water_works);		ADDHASH(wetland);
	ADDHASH(wilderness_hut);	ADDHASH(wildlife_park);		ADDHASH(winery);
	ADDHASH(winter_sports);		ADDHASH(wood);				ADDHASH(works);
	ADDHASH(zoo);
	// Transit
	ADDHASH(light_rail);		ADDHASH(platform);			//ADDHASH(railway);
	ADDHASH(subway);			ADDHASH(train);				ADDHASH(tram);


	// Water
	ADDHASH(basin);				ADDHASH(bay);				ADDHASH(canal);
	ADDHASH(ditch);				ADDHASH(dock);				ADDHASH(drain);
	ADDHASH(fjord);				ADDHASH(lake);				ADDHASH(ocean);
	ADDHASH(playa);				ADDHASH(river);				ADDHASH(riverbank);
	ADDHASH(sea);				ADDHASH(stream);			ADDHASH(strait);
	ADDHASH(swimming_pool);		ADDHASH(water);
#undef ADDHASH

	for (int i = 0; i < NUM_KINDS; i++)
	{
		assert(gKindHash[i].val == i);
	}
}
//-----------------------------------------------------------------------------
static inline bool extractPoint(const rapidjson::Value& _in, Point& p, const Tile& _tile, Point* last=NULL)
{
    const Vec2d pos = LonLatToMeters(Vec2d(_in[0].GetDouble(), _in[1].GetDouble()));
	p.x = (pos.x - _tile.tileOrigin.x);
	p.y = (pos.y - _tile.tileOrigin.y);
	p.z = 0;
    if (last && myLength(p - *last) < 1e-5f)
        return false;
    
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
bool SkipFeature(const ELayerType layerType, const Feature& f)
{
	bool skip = false;

	// Skip water boundaries
	if (layerType == eLayerWater)
	{
		if (f.boundary)
			skip = true;
	}

	// Skip other geometry than lines & polygons
	if ((f.geometryType == GeometryType::unknown || f.geometryType == GeometryType::points))
		skip = true;

	//if(skip)
	//	LOG("Skipped feature (%I64d)\n", f.id);
	
	return skip;
}
//-----------------------------------------------------------------------------
bool GeoJson::extractFeature(const ELayerType layerType, const rapidjson::Value& _in, Feature& f, const Tile& _tile, const float defaultHeight)
{
	static bool hashCreated = false;
	if (!hashCreated)
	{
		hashCreated = true;
		CreateHash();
	}

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
		return false;

	// Copy geometry into tile data
    if (geomType == "Point")
	{
        f.points.emplace_back();
        if (!extractPoint(coords, f.points.back(), _tile)) { f.points.pop_back(); }
    }
	else if (geomType == "MultiPoint")
	{
        for (auto pointCoords = coords.Begin(); pointCoords != coords.End(); ++pointCoords) {
            if (!extractPoint(coords, f.points.back(), _tile)) { f.points.pop_back(); }
        }
    } 
	else if (geomType == "LineString")
	{
        f.lineStrings.emplace_back();
        extractLineString(coords, f.lineStrings.back(), _tile);
    } 
	else if (geomType == "MultiLineString")
	{
        for (auto lineCoords = coords.Begin(); lineCoords != coords.End(); ++lineCoords)
		{
            f.lineStrings.emplace_back();
            extractLineString(*lineCoords, f.lineStrings.back(), _tile);
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
// Parsed json in 81.7ms. Built GeoJson structures in 74.0ms
// Parsed json in 80.0ms.Built GeoJson structures in 76.2ms
// Parsed json in 79.2ms.Built GeoJson structures in 75.2ms
// Parsed json in 2.1ms.Built GeoJson structures in 2.0ms
// Parsed json in 7.0ms.Built GeoJson structures in 6.2ms
// Parsed json in 2.0ms.Built GeoJson structures in 3.2ms
//
// Parsed json in 77.0ms. Built GeoJson structures in 72.5ms
// Parsed json in 78.8ms.Built GeoJson structures in 75.6ms
// Parsed json in 64.5ms.Built GeoJson structures in 75.4ms
// Parsed json in 7.3ms.Built GeoJson structures in 8.0ms
// Parsed json in 2.8ms. Built GeoJson structures in 2.1ms
// Parsed json in 2.0ms. Built GeoJson structures in 1.8ms
//
// Parsed json in 82.7ms. Built GeoJson structures in 53.4ms
// Parsed json in 74.3ms.Built GeoJson structures in 51.0ms
// Parsed json in 67.8ms.Built GeoJson structures in 48.7ms
// Parsed json in 2.0ms. Built GeoJson structures in 1.3ms
// Parsed json in 7.2ms. Built GeoJson structures in 5.9ms
// Parsed json in 6.9ms. Built GeoJson structures in 4.7ms
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
