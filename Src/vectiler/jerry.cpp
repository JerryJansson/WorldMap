#include "Precompiled.h"
#include "..\..\..\..\Source\Modules\Shared\SceneNew.h"
#include "vectiler.h"
#include "geometry.h"
#include "tilemanager.h"
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
Tile::Tile(int x, int y, int z) : x(x), y(y), z(z)
{
	Vec4d bounds = TileBounds(Vec2i(x, y), z);
	tileOrigin = bounds.xy();
	borders = 0;
}
//-----------------------------------------------------------------------------
bool GetTile(MyTile* t, TArray<GGeom>& geoms)
{
	assert(t->Status() == MyTile::eNotLoaded);

	const Vec3i& tms = t->m_Tms;
	const CStrL tileName = Str_Printf("%d_%d_%d", tms.x, tms.y, tms.z);
	const CStrL fname = tileName + ".bin";

	Vec2i google = TmsToGoogleTile(Vec2i(tms.x, tms.y), tms.z);
	LOG("GetTile tms: <%d,%d,%d>, google: <%d, %d>\n", tms.x, tms.y, tms.z, google.x, google.y);

	if (tile_DiscCache)
	{
		if (LoadBin(fname, geoms))
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

	if (!LoadBin(fname, geoms))
		return false;

	return true;
}