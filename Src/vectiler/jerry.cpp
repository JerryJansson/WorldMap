#include "Precompiled.h"
#include "..\..\..\..\Source\Modules\Shared\SceneNew.h"
#include "vectiler.h"
#include "geometry.h"
#include "tilemanager.h"
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
	//tileOrigin = v2d(0.5 * (bounds.x + bounds.z), -0.5 * (bounds.y + bounds.w));
	//tileOrigin = v2d(0.5 * (bounds.x + bounds.z), 0.5*(bounds.y + bounds.w));
	tileOrigin = bounds.xy();

	// JJ
	//double scale = 0.5 * myAbs(bounds.x - bounds.z);
	//invScale = 1.0 / scale;
	//invScale = 1.0f;

	borders = 0;
}
//-----------------------------------------------------------------------------
/*MyTile* GetTile2(const Vec3i tms)
{
	const CStrL tileName = Str_Printf("%d_%d_%d", tms.x, tms.y, tms.z);
	const CStrL fname = tileName + ".bin";

	Vec2i google = TmsToGoogleTile(Vec2i(tileKey.x, tms.y), tms.z);
	LOG("GetTile tms: <%d,%d>, google: <%d, %d>\n", tms.x, tms.y, google.x, google.y);
	
	//if (LoadBin(fname))
	//	return true;
	

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
		true,					// buildings. Whether to export building geometry
		10.0f,					// buildingsHeight
		1.0f,					// buildingsExtrusionScale
		true,					// roads. Whether to export roads geometry
		0.2f,					// roadsHeight
		3.0f					// roadsExtrusionWidth,
	};
	
	if (!vectiler2(params))
		return NULL;

	TStackArray<GGeom, 64> geoms;
	if (!LoadBin(fname, geoms))
		return NULL;

	MyTile* tileEntity = new MyTile(tileKey);

	for (int i = 0; i < geoms.Num(); i++)
	{
		CMesh* mesh = CreateMeshFromGeoms(geoms[i].name, &geoms[i], 1);
		Entity* e = new Entity(geoms[i].name);
		MeshComponent* meshcomp = e->CreateComponent<MeshComponent>();
		meshcomp->m_DrawableFlags.Set(Drawable::eLightMap);
		meshcomp->SetMesh(mesh, MeshComponent::eMeshDelete);
		//e->SetPos(CVec3(0, 10, 0));
		//gScene.AddEntity(entity);
		tileEntity->AddChild(e);
	}

	gScene.AddEntity(tileEntity);
	return tileEntity;
}*/

bool GetTile3(MyTile* t, TArray<GGeom>& geoms)
{
	assert(t->Status() == MyTile::eNotLoaded);

	const Vec3i& tms = t->m_Tms;
	const CStrL tileName = Str_Printf("%d_%d_%d", tms.x, tms.y, tms.z);
	const CStrL fname = tileName + ".bin";

	Vec2i google = TmsToGoogleTile(Vec2i(tms.x, tms.y), tms.z);
	LOG("GetTile tms: <%d,%d>, google: <%d, %d>\n", tms.x, tms.y, google.x, google.y);

	//if (LoadBin(fname, geoms))
	//	return true;

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
		true,					// buildings. Whether to export building geometry
		//10.0f,					// buildingsHeight
		//1.0f,					// buildingsExtrusionScale
		true					// roads. Whether to export roads geometry
		//0.2f,					// roadsHeight
		//3.0f					// roadsExtrusionWidth,
	};

	if (!vectiler2(params))
		return false;

	if (!LoadBin(fname, geoms))
		return false;

	return true;
}