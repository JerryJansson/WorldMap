#include "Precompiled.h"
#include "geojson.h"
//#include "jerry.h"
Hash<CStr, EFeatureKind> gKindHash;
//-----------------------------------------------------------------------------
void CreateHash()
{
	gKindHash.Reserve(NUM_KINDS);

#define ADDHASH(KIND) gKindHash.Add(#KIND, eKind_##KIND)
	ADDHASH(unknown);

	// Buildings
	gKindHash.Add("building", eKindBuilding);
	gKindHash.Add("building_part", eKindBuildingPart);
	gKindHash.Add("address", eKindAddress);

	// Roads
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
	ADDHASH(aerodrome);
	ADDHASH(allotments);
	ADDHASH(amusement_ride);
	ADDHASH(animal);
	ADDHASH(apron);
	ADDHASH(aquarium);
	ADDHASH(artwork);
	ADDHASH(attraction);
	ADDHASH(aviary);
	ADDHASH(battlefield);
	ADDHASH(beach);
	ADDHASH(breakwater);
	ADDHASH(bridge);
	ADDHASH(camp_site);
	ADDHASH(caravan_site);
	ADDHASH(carousel);
	ADDHASH(cemetery);
	ADDHASH(cinema);
	ADDHASH(city_wall);
	ADDHASH(college);
	ADDHASH(commercial);
	ADDHASH(common);
	ADDHASH(cutline);
	ADDHASH(dam);
	ADDHASH(dike);
	ADDHASH(dog_park);
	ADDHASH(enclosure);
	ADDHASH(farm);
	ADDHASH(farmland);
	ADDHASH(farmyard);
	ADDHASH(fence);
	ADDHASH(footway);
	ADDHASH(forest);
	ADDHASH(fort);
	ADDHASH(fuel);
	ADDHASH(garden);
	ADDHASH(gate);
	ADDHASH(generator);
	ADDHASH(glacier);
	ADDHASH(golf_course);
	ADDHASH(grass);
	ADDHASH(grave_yard);
	ADDHASH(groyne);
	ADDHASH(hanami);
	ADDHASH(hospital);
	ADDHASH(industrial);
	ADDHASH(land);
	ADDHASH(library);
	ADDHASH(maze);
	ADDHASH(meadow);
	ADDHASH(military);
	ADDHASH(national_park);
	ADDHASH(nature_reserve);
	ADDHASH(natural_forest);
	ADDHASH(natural_park);
	ADDHASH(natural_wood);
	ADDHASH(park);
	ADDHASH(parking);
	ADDHASH(pedestrian);
	ADDHASH(petting_zoo);
	ADDHASH(picnic_site);
	ADDHASH(pier);
	ADDHASH(pitch);
	ADDHASH(place_of_worship);
	ADDHASH(plant);
	ADDHASH(playground);
	ADDHASH(prison);
	ADDHASH(protected_area);
	ADDHASH(quarry);
	ADDHASH(railway);
	ADDHASH(recreation_ground);
	ADDHASH(recreation_track);
	ADDHASH(residential);
	ADDHASH(resort);
	ADDHASH(rest_area);
	ADDHASH(retail);
	ADDHASH(retaining_wall);
	ADDHASH(rock);
	ADDHASH(roller_coaster);
	ADDHASH(runway);
	ADDHASH(rural);
	ADDHASH(school);
	ADDHASH(scree);
	ADDHASH(scrub);
	ADDHASH(service_area);
	ADDHASH(snow_fence);
	ADDHASH(sports_centre);
	ADDHASH(stadium);
	ADDHASH(stone);
	ADDHASH(substation);
	ADDHASH(summer_toboggan);
	ADDHASH(taxiway);
	ADDHASH(theatre);
	ADDHASH(theme_park);
	ADDHASH(tower);
	ADDHASH(trail_riding_station);
	ADDHASH(university);
	ADDHASH(urban_area);
	ADDHASH(urban);
	ADDHASH(village_green);
	ADDHASH(wastewater_plant);
	ADDHASH(water_park);
	ADDHASH(water_slide);
	ADDHASH(water_works);
	ADDHASH(wetland);
	ADDHASH(wilderness_hut);
	ADDHASH(wildlife_park);
	ADDHASH(winery);
	ADDHASH(winter_sports);
	ADDHASH(wood);
	ADDHASH(works);
	ADDHASH(zoo);

	// Transit
	ADDHASH(light_rail);
	ADDHASH(platform);
	//ADDHASH(railway);
	ADDHASH(subway);
	ADDHASH(train);
	ADDHASH(tram);


	// Water
	gKindHash.Add("basin", eWaterBasin);
	gKindHash.Add("bay", eWaterBay);
	gKindHash.Add("canal", eWaterCanal);
	gKindHash.Add("ditch", eWaterDitch);
	gKindHash.Add("dock", eWaterDock);
	gKindHash.Add("drain", eWaterDrain);
	gKindHash.Add("fjord", eWaterFjord);
	gKindHash.Add("lake", eWaterLake);
	gKindHash.Add("ocean", eWaterOcean);
	gKindHash.Add("playa", eWaterPlaya);
	gKindHash.Add("river", eWaterRiver);
	gKindHash.Add("riverbank", eWaterRiverbank);
	gKindHash.Add("sea", eWaterSea);
	gKindHash.Add("stream", eWaterStream);
	gKindHash.Add("strait", eWaterStrait);
	gKindHash.Add("swimming_pool", eWaterSwimming_pool);
	gKindHash.Add("water", eWaterWater);
#undef ADDHASH

	for (int i = 0; i < NUM_KINDS; i++)
	{
		assert(gKindHash[i].val == i);
	}
}
//-----------------------------------------------------------------------------
static inline bool extractPoint(const rapidjson::Value& _in, Point& _out, const Tile& _tile, Point* last=NULL)
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

	CStr tmpstr;

    for (auto itr = properties.MemberBegin(); itr != properties.MemberEnd(); ++itr)
	{
        const auto& member = itr->name.GetString();
        const rapidjson::Value& prop = properties[member];
		
		if (strcmp(member, "height") == 0)			_out.height		= prop.GetDouble();
        else if (strcmp(member, "min_height") == 0) _out.min_height = prop.GetDouble();
		else if (strcmp(member, "sort_rank") == 0)  _out.sort_rank	= prop.GetInt();
		else if (strcmp(member, "name") == 0)		_out.name		= prop.GetString();
		else if (strcmp(member, "id") == 0)			_out.id			= prop.GetInt64(); // Can be signed
		else if (strcmp(member, "kind") == 0)
		{
			tmpstr = prop.GetString();
			_out.kind = gKindHash.Get(tmpstr);

			if(/*layerType==eLayerLanduse &&*/ _out.kind == eKind_unknown)
			{
				int abba = 10;
			}
		}
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
