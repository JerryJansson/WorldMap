#include "Precompiled.h"
#include "mapping.h"
#include "projection.h"
//-----------------------------------------------------------------------------
Crgba gKindColors[NUM_KINDS];
Hash<CStr, EFeatureKind> gKindHash;
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
}
//-----------------------------------------------------------------------------
void InitializeColors()
{
	for (int i = 0; i < NUM_KINDS; i++)
	{
		Crgba& c = gKindColors[i];
		c = gRGBA_Red;

		// Buildings
		if (i >= eKind_building && i <= eKind_address)
		{
			if (i == eKind_building) c = Crgba(217, 208, 201, 255);
			else if (i == eKind_building_part) c = Crgba(0, 208, 201, 255);
		}
		// Earth
		else if (i == eKind_earth)
		{
			c = Crgba(200, 200, 200, 255);
		}
		// Transit
		else if (i >= eKind_light_rail && i <= eKind_tram)
		{
			//c.Random();
		}
		// Roads
		else if (i >= eKind_highway && i <= eKind_rail)//eKindPortageway)
		{
			if (i == eKind_highway)			c = Crgba(241, 188, 198);
			else if (i == eKind_major_road) c = Crgba(252, 214, 164);
			else if (i == eKind_minor_road) c = Crgba(255, 255, 255);
			else if (i == eKind_path)		c = Crgba(154, 154, 154);
			//else if (i == eKind_rail)		c = Crgba(0, 0, 154);
		}
		// Landuse
		else if (i >= eKind_aerodrome && i <= eKind_zoo)
		{
			switch (i)
			{
			case eKind_park: c = Crgba(200, 250, 204); break;
			case eKind_grass: c = Crgba(200, 235, 176); break;
			case eKind_garden: c = Crgba(215, 235, 176); break;
			case eKind_pedestrian: c = Crgba(221, 221, 232); break;
			case eKind_retaining_wall: c = Crgba(180, 180, 180); break;	// ???
			case eKind_fence: c = Crgba(138, 163, 140); break;
			}
		}
		// Water
		else if (i >= eKind_basin && i <= eKind_water)
		{
			if (i == eKind_basin)			c = Crgba(181, 208, 208);
			else if(i== eKind_ocean)		c = Crgba(181, 208, 208);
			else if (i == eKind_riverbank)	c = Crgba(181, 208, 208);
			else if (i == eKind_water)		c = Crgba(181, 208, 208);
		}
	}
}
//-----------------------------------------------------------------------------
Tile::Tile(int x, int y, int z) : x(x), y(y), z(z)
{
	Vec4d bounds = TileBounds(Vec2i(x, y), z);
	tileOrigin = bounds.xy();
	borders = 0;
}