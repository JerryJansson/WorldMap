#include "Precompiled.h"
#include "mapping.h"
#include "projection.h"
//-----------------------------------------------------------------------------
Hash<CStr, EFeatureKind> gKindHash;
Hash<int, MapGeom> gGeomHash;
//-----------------------------------------------------------------------------
const char* layerNames[eNumLayerTypes + 1] =
{
	"unknown",
	"terrain",
	"water",
	"buildings",
	"places",
	"transit",
	"pois",
	"boundaries",
	"roads",
	"earth",
	"landuse",

	NULL		// For IndexFromStringTable
};
//-----------------------------------------------------------------------------
void CreateKindHash()
{
	gKindHash.Reserve(NUM_KINDS);

#define ADDHASH(KIND) gKindHash.Add(#KIND, eKind_##KIND)
	ADDHASH(unknown);

	// Buildings
	ADDHASH(building);			ADDHASH(building_part);		ADDHASH(address);
	// Earth
	ADDHASH(archipelago);		ADDHASH(arete);				ADDHASH(cliff);
	ADDHASH(continent);			ADDHASH(earth);				ADDHASH(island);
	ADDHASH(islet);				ADDHASH(ridge);				ADDHASH(valley);
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
	// Pois
	ADDHASH(station);
	//Boundaries
	ADDHASH(aboriginal_lands);	ADDHASH(country);			ADDHASH(county);
	ADDHASH(disputed);			ADDHASH(indefinite);		ADDHASH(indeterminate);
	ADDHASH(lease_limit);		ADDHASH(line_of_control);	ADDHASH(locality);
	ADDHASH(macroregion);		ADDHASH(map_unit);			ADDHASH(overlay_limit);
	ADDHASH(region);

#undef ADDHASH

	for (int i = 0; i < NUM_KINDS; i++)
	{
		assert(gKindHash[i].val == i);
	}


	gGeomHash.Reserve(NUM_KINDS);
	ELayerType l;
	
#define ADDHASH(KIND, C)		gGeomHash.Add((l<<16)|eKind_##KIND, MapGeom(C, 0.1f, false))
#define ADDHASH2(KIND, C, W)	gGeomHash.Add((l<<16)|eKind_##KIND, MapGeom(C, W, false))
#define ADDHASH3(KIND, C, W, S) gGeomHash.Add((l<<16)|eKind_##KIND, MapGeom(C, W, S))
	
	l = eLayerWater;
	ADDHASH(basin,			Crgba(181, 208, 208));
	ADDHASH2(canal,			Crgba(181, 208, 208),	1.5f);
	ADDHASH(ocean,			Crgba(181, 208, 208));
	ADDHASH2(river,			Crgba(181, 208, 208),	2.0f);
	ADDHASH(riverbank,		Crgba(181, 208, 208));
	ADDHASH2(stream,		Crgba(181, 208, 208),	1.0f);
	ADDHASH(water,			Crgba(181, 208, 208));

	l = eLayerBuildings;
	ADDHASH(building,		Crgba(217, 208, 201));
	ADDHASH(building_part,	Crgba(0, 208, 201));

	l = eLayerTransit;
	ADDHASH3(light_rail,	Crgba(255, 0, 0),		0.5f,	true);
	ADDHASH3(subway,		Crgba(255, 0, 0),		0.5f,	true);
	ADDHASH3(train,			Crgba(255, 0, 0),		0.5f,	true);
	
	l = eLayerBoundaries;
	ADDHASH3(locality,		Crgba(255, 0, 0),		0.5f,	true);
	ADDHASH3(county,		Crgba(255, 0, 0),		0.5f,	true);
	ADDHASH3(region,		Crgba(255, 0, 0),		0.5f,	true);
	
	l = eLayerRoads;
	ADDHASH2(aerialway,		Crgba(241, 100, 198),	6);
	ADDHASH2(aeroway,		Crgba(241, 100, 198),	8);
	ADDHASH3(ferry,			Crgba(241, 100, 198),	1,		true);
	ADDHASH2(highway,		Crgba(241, 188, 198),	6);
	ADDHASH2(major_road,	Crgba(252, 214, 164),	5);
	ADDHASH2(minor_road,	Crgba(255, 255, 255),	4);
	ADDHASH2(path,			Crgba(154, 154, 154),	0.5f);
	ADDHASH2(piste,			Crgba(241, 100, 198),	5);
	ADDHASH2(racetrack,		Crgba(241, 100, 198),	3);
	ADDHASH3(rail,			Crgba(241, 100, 198),	0.4f,	true);
	
	l = eLayerEarth;
	ADDHASH3(cliff,			Crgba(255, 0, 0),		0.1f,	true);
	ADDHASH(earth,			Crgba(200, 200, 200));
	
	l = eLayerLanduse;
	ADDHASH(cemetery,		Crgba(128, 128, 128));
	ADDHASH2(gate,			Crgba(255, 0, 0), 10.15f);
	ADDHASH(grass,			Crgba(200, 235, 176));
	ADDHASH(garden,			Crgba(215, 235, 176));
	ADDHASH(park,			Crgba(200, 250, 204));
	ADDHASH(parking,		Crgba(180, 160, 204));
	ADDHASH(pedestrian,		Crgba(221, 221, 232));
	ADDHASH3(protected_area,Crgba(255, 0, 0),		0.5,	true);
	ADDHASH2(retaining_wall,Crgba(180, 180, 180),	0.5f);	// ???
	ADDHASH2(fence,			Crgba(138, 163, 140),	0.15f);

#undef ADDHASH3
#undef ADDHASH2
#undef ADDHASH
}
//-----------------------------------------------------------------------------
Tile::Tile(int x, int y, int z) : x(x), y(y), z(z)
{
	Vec4d bounds = TileBounds(Vec2i(x, y), z);
	tileOrigin = bounds.xy();
	borders = 0;
}