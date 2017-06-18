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
	// Earth
	ADDHASH(earth);
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
void GeoJson::extractLineString(const rapidjson::Value& arr, LineString& l, const Tile& _tile)
{
	const int count = arr.Size();

	if (count == 0)
		return;

	l.reserve(arr.Size());
	l.emplace_back();
	extractP(arr[0], l.back(), _tile);

	for (int i=1; i<count; i++)
	{
		l.emplace_back();
		if (!extractPoint(arr[i], l.back(), _tile, &l[l.size() - 2]))
			l.pop_back();
	}
}
//-----------------------------------------------------------------------------
/*void GeoJson::extractPoly(const rapidjson::Value& _in, Polygon2& _out, const Tile& _tile)
{
    for (auto itr = _in.Begin(); itr != _in.End(); ++itr)
	{
        _out.emplace_back();
        extractLineString(*itr, _out.back(), _tile);
    }
}*/
//-----------------------------------------------------------------------------
// A poly is 1 or more linestrings
// First linestring can be poly, second linestring can be a hole
//-----------------------------------------------------------------------------
void GeoJson::extractPoly(const rapidjson::Value& arr, Polygon2& poly, const Tile& _tile)
{
	const int count = arr.Size();
	poly.reserve(count);
	for(int i=0; i<count; i++)
	{
		poly.emplace_back();
		extractLineString(arr[i], poly.back(), _tile);
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
	else if (layerType == eLayerRoads)
	{
		if (f.kind == eKind_ferry) skip = true;			// Ferry lines
		else if (f.kind == eKind_rail) skip = true;		// Train rails
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
			/*if (f.kind == eKind_unknown)
			{
				int abba = 01;
			}*/
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
