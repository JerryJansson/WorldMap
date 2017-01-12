#include "Precompiled.h"
#include "..\..\..\..\Source\Modules\Shared\SceneNew.h"
#include "vectiler.h"
#include "geometry.h"
#include "tilemanager.h"
#include "disc.h"
//-----------------------------------------------------------------------------
extern CVar tile_DiscCache;
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
Crgba kindColors[NUM_KINDS];
//-----------------------------------------------------------------------------
void InitializeColors()
{
	for (int i = 0; i < NUM_KINDS; i++)
	{
		kindColors[i] = gRGBA_Red;

		// Buildings
		if (i >= eKindBuilding && i <= eKindAddress)
		{
			kindColors[i] = Crgba(180, 180, 180, 255);
		}
		// Roads
		else if (i >= eKindHighway && i <= eKindPortageway)
		{
			kindColors[i] = Crgba(80, 80, 60, 255);
		}
		// Landuse
		else if(KIND_IS_LANDUSE(i))
		{
			kindColors[i].Random();// = Crgba(128, 200, 32, 255);
		}
		// Water
		else if (i >= eWaterBasin && i <= eWaterWater)
		{
			kindColors[i] = Crgba(32, 128, 190, 255);
		}
		else
		{
			kindColors[i].Random();
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
//-----------------------------------------------------------------------------
bool GetTile(StreamResult* result)
{
	const MyTile* t = result->tile;

	assert(t->Status() == MyTile::eNotLoaded);

	const Vec3i& tms = t->m_Tms;
	const CStrL tileName = Str_Printf("%d_%d_%d", tms.x, tms.y, tms.z);
	const CStrL fname = tileName + ".bin";

	Vec2i google = TmsToGoogleTile(Vec2i(tms.x, tms.y), tms.z);
	//LOG("GetTile tms: <%d,%d,%d>, google: <%d, %d>\n", tms.x, tms.y, tms.z, google.x, google.y);

	if (tile_DiscCache)
	{
		if (LoadBin(fname, result->geoms))
			return true;
	}

	// Mapzen uses google xyz indexing
	struct Params2 params =
	{
		"vector-tiles-qVaBcRA",	// apiKey
		tms.x,					// Tile X
		tms.y,					// Tile Y
		tms.z,					// Tile Z (zoom)
		false,					// terrain. Generate terrain elevation topography
		64,						// terrainSubdivision
		1.0f,					// terrainExtrusionScale
		true					// vectorData. Buildings, roads, landuse, pois, etc...
	};

	if (!vectiler(params))
		return false;

	if (!LoadBin(fname, result->geoms))
		return false;

	return true;
}