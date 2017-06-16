#include "Precompiled.h"
#include "..\..\..\..\Source\Modules\Shared\SceneNew.h"
#include "vectiler.h"
#include "geometry.h"
#include "tilemanager.h"
#include "disc.h"
//-----------------------------------------------------------------------------
extern CVar tile_DiscCache;

//-----------------------------------------------------------------------------
bool GetTile(StreamResult* result)
{
	const MyTile* t = result->tile;

	assert(t->Status() == MyTile::eNotLoaded);

	const Vec3i& tms = t->m_Tms;
	const CStrL tileName = Str_Printf("%d_%d_%d", tms.x, tms.y, tms.z);
	const CStrL fname = tileName + ".bin";

	//Vec2i google = TmsToGoogleTile(Vec2i(tms.x, tms.y), tms.z);
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