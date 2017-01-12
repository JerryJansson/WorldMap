#pragma once
//-----------------------------------------------------------------------------
#include <bitset>
//-----------------------------------------------------------------------------
enum ELayerType
{
	eLayerUnknown,		// 0

	eLayerTerrain,		// 1
	eLayerWater,		// 2
	eLayerBuildings,	// 3
	eLayerPlaces,		// 4
	eLayerTransit,		// 5
	eLayerPois,			// 6
	eLayerBoundaries,	// 7
	eLayerRoads,		// 8
	eLayerEarth,		// 9
	eLayerLanduse,		// 10

	eNumLayerTypes
};
//-----------------------------------------------------------------------------
enum EFeatureKind
{
	eKind_unknown,

	// Buildings
	eKindBuilding,
	eKindBuildingPart,
	eKindAddress,

	// Roads
	eKindHighway,
	eKindMajorRoad,
	eKindMinorRoad,
	eKindRail,
	eKindPath,
	eKindFerry,
	eKindPiste,
	eKindAerialway,
	eKindAeroway,
	eKindRacetrack,
	eKindPortageway,

	// Landuse
	eKind_aerodrome,
	eKind_allotments,
	eKind_amusement_ride,
	eKind_animal,
	eKind_apron,
	eKind_aquarium,
	eKind_artwork,
	eKind_attraction,
	eKind_aviary,
	eKind_battlefield,
	eKind_beach,
	eKind_breakwater,
	eKind_bridge,
	eKind_camp_site,
	eKind_caravan_site,
	eKind_carousel,
	eKind_cemetery,
	eKind_cinema,
	eKind_city_wall,
	eKind_college,
	eKind_commercial,
	eKind_common,
	eKind_cutline,
	eKind_dam,
	eKind_dike,
	eKind_dog_park,
	eKind_enclosure,
	eKind_farm,
	eKind_farmland,
	eKind_farmyard,
	eKind_fence,
	eKind_footway,
	eKind_forest,
	eKind_fort,
	eKind_fuel,
	eKind_garden,
	eKind_gate,
	eKind_generator,
	eKind_glacier,
	eKind_golf_course,
	eKind_grass,
	eKind_grave_yard,
	eKind_groyne,
	eKind_hanami,
	eKind_hospital,
	eKind_industrial,
	eKind_land,
	eKind_library,
	eKind_maze,
	eKind_meadow,
	eKind_military,
	eKind_national_park,
	eKind_nature_reserve,
	eKind_natural_forest,
	eKind_natural_park,
	eKind_natural_wood,
	eKind_park,
	eKind_parking,
	eKind_pedestrian,
	eKind_petting_zoo,
	eKind_picnic_site,
	eKind_pier,
	eKind_pitch,
	eKind_place_of_worship,
	eKind_plant,
	eKind_playground,
	eKind_prison,
	eKind_protected_area,
	eKind_quarry,
	eKind_railway,
	eKind_recreation_ground,
	eKind_recreation_track,
	eKind_residential,
	eKind_resort,
	eKind_rest_area,
	eKind_retail,
	eKind_retaining_wall,
	eKind_rock,
	eKind_roller_coaster,
	eKind_runway,
	eKind_rural,
	eKind_school,
	eKind_scree,
	eKind_scrub,
	eKind_service_area,
	eKind_snow_fence,
	eKind_sports_centre,
	eKind_stadium,
	eKind_stone,
	eKind_substation,
	eKind_summer_toboggan,
	eKind_taxiway,
	eKind_theatre,
	eKind_theme_park,
	eKind_tower,
	eKind_trail_riding_station,
	eKind_university,
	eKind_urban_area,
	eKind_urban,
	eKind_village_green,
	eKind_wastewater_plant,
	eKind_water_park,
	eKind_water_slide,
	eKind_water_works,
	eKind_wetland,
	eKind_wilderness_hut,
	eKind_wildlife_park,
	eKind_winery,
	eKind_winter_sports,
	eKind_wood,
	eKind_works,
	eKind_zoo,

	// Transit
	eKind_light_rail,
	eKind_platform,
	//eKind_railway,
	eKind_subway,
	eKind_train,
	eKind_tram,

	// Water
	eWaterBasin,
	eWaterBay,
	eWaterCanal,
	eWaterDitch,
	eWaterDock,
	eWaterDrain,
	eWaterFjord,
	eWaterLake,
	eWaterOcean,
	eWaterPlaya,
	eWaterRiver,
	eWaterRiverbank,
	eWaterSea,
	eWaterStream,
	eWaterStrait,
	eWaterSwimming_pool,
	eWaterWater,

	NUM_KINDS
};
//-----------------------------------------------------------------------------
// Temporary!
//-----------------------------------------------------------------------------
#define KIND_IS_LANDUSE(k)	(k>=eKind_aerodrome && k<=eKind_zoo)
#define KIND_IS_TRANSIT(k)	(k>=eKind_light_rail && k<=eKind_tram)
#define KIND_IS_WATER(k)	(k>=eWaterBasin && k<=eWaterWater)
//-----------------------------------------------------------------------------
struct Tile {
	int x;
	int y;
	int z;

	std::bitset<4> borders;
	Vec2d tileOrigin;

	bool operator==(const Tile& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

	Tile(int x, int y, int z);
};
//-----------------------------------------------------------------------------
extern const char* layerNames[eNumLayerTypes + 1];
extern Crgba kindColors[NUM_KINDS];
//-----------------------------------------------------------------------------
void InitializeColors();