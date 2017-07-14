#include "Precompiled.h"
#include "tiledata.h"
#include "vectiler.h"
#include "download.h"
#include "geometry.h"
#include "disc.h"
//-----------------------------------------------------------------------------
extern CVar tile_DiscCache;
//-----------------------------------------------------------------------------
enum Border { right, left, bottom, top, };
//-----------------------------------------------------------------------------
void buildPlane(VtxArr& outVertices, IdxArr& outIndices,
	float width,       // Total plane width (x-axis)
	float height,      // Total plane height (y-axis)
	unsigned int nw,   // Split on width
	unsigned int nh,   // Split on height
	bool flip = false)
{
	// TODO: add offsets
	//std::vector<v4> vertices;
	//std::vector<int> indices;

	int indexOffset = 0;

	float ow = width / nw;
	float oh = height / nh;
	static const v3 up(0.0, 0.0, 1.0);

	v3 normal = up;

	if (flip) {
		normal *= -1.f;
	}

	for (float w = -width / 2.0; w <= width / 2.0 - ow; w += ow) {
		for (float h = -height / 2.0; h <= height / 2.0 - oh; h += oh) {
			v3 v0(w, h + oh, 0.0);
			v3 v1(w, h, 0.0);
			v3 v2(w + ow, h, 0.0);
			v3 v3(w + ow, h + oh, 0.0);

			outVertices.Add({ v0, normal });
			outVertices.Add({ v1, normal });
			outVertices.Add({ v2, normal });
			outVertices.Add({ v3, normal });

			if (!flip) {
				outIndices.Add(indexOffset + 0);
				outIndices.Add(indexOffset + 1);
				outIndices.Add(indexOffset + 2);
				outIndices.Add(indexOffset + 0);
				outIndices.Add(indexOffset + 2);
				outIndices.Add(indexOffset + 3);
			}
			else {
				outIndices.Add(indexOffset + 0);
				outIndices.Add(indexOffset + 2);
				outIndices.Add(indexOffset + 1);
				outIndices.Add(indexOffset + 0);
				outIndices.Add(indexOffset + 3);
				outIndices.Add(indexOffset + 2);
			}

			indexOffset += 4;
		}
	}
}

//-----------------------------------------------------------------------------
void buildPedestalPlanes(const Tile& tile, VtxArr& outVertices,	IdxArr& outIndices, const HeightData* elevation, unsigned int subdiv, float pedestalHeight)
{
	const float offset = 1.0 / subdiv;
	const v3 upVector(0.0, 0.0, 1.0);
	int vertexDataOffset = outVertices.size();

	for (size_t i = 0; i < tile.borders.size(); ++i)
	{
		if (!tile.borders[i]) continue;

		for (float x = -1.0; x < 1.0; x += offset)
		{
			v3 v0, v1, normalVector;

			if (i == Border::right) { v0 = v3(1.0, x + offset, 0.0);	v1 = v3(1.0, x, 0.0); }
			if (i == Border::left) { v0 = v3(-1.0, x + offset, 0.0);	v1 = v3(-1.0, x, 0.0); }
			if (i == Border::top) { v0 = v3(x + offset, 1.0, 0.0);	v1 = v3(x, 1.0, 0.0); }
			if (i == Border::bottom) { v0 = v3(x + offset, -1.0, 0.0);	v1 = v3(x, -1.0, 0.0); }

			normalVector = cross(upVector, v0 - v1);
			normalVector = Normalize(normalVector);

			float h0 = sampleElevation(v2(v0.x, v0.y), elevation);
			float h1 = sampleElevation(v2(v1.x, v1.y), elevation);

			v0.z = h0;// *tile.invScale;
			outVertices.push_back({ v0, normalVector });
			v1.z = h1;// *tile.invScale;
			outVertices.push_back({ v1, normalVector });
			v0.z = pedestalHeight;// *tile.invScale;
			outVertices.push_back({ v0, normalVector });
			v1.z = pedestalHeight;// *tile.invScale;
			outVertices.push_back({ v1, normalVector });

			if (i == Border::right || i == Border::bottom) {
				outIndices.push_back(vertexDataOffset + 0);
				outIndices.push_back(vertexDataOffset + 1);
				outIndices.push_back(vertexDataOffset + 2);
				outIndices.push_back(vertexDataOffset + 1);
				outIndices.push_back(vertexDataOffset + 3);
				outIndices.push_back(vertexDataOffset + 2);
			}
			else {
				outIndices.push_back(vertexDataOffset + 0);
				outIndices.push_back(vertexDataOffset + 2);
				outIndices.push_back(vertexDataOffset + 1);
				outIndices.push_back(vertexDataOffset + 1);
				outIndices.push_back(vertexDataOffset + 2);
				outIndices.push_back(vertexDataOffset + 3);
			}

			vertexDataOffset += 4;
		}
	}
}
//-----------------------------------------------------------------------------
/*void adjustTerrainEdges(std::unordered_map<Tile, HeightData*>& heightData)
{
	for (auto& tileData0 : heightData)
	{
		auto& tileHeight0 = tileData0.second;

		for (auto& tileData1 : heightData) {
			if (tileData0.first == tileData1.first) {
				continue;
			}

			auto& tileHeight1 = tileData1.second;

			if (tileData0.first.x + 1 == tileData1.first.x && tileData0.first.y == tileData1.first.y)
			{
				for (int y = 0; y < tileHeight0->height; ++y)
				{
					float h0 = tileHeight0->elevation[tileHeight0->width - 1][y];
					float h1 = tileHeight1->elevation[0][y];
					float h = (h0 + h1) * 0.5f;
					tileHeight0->elevation[tileHeight0->width - 1][y] = h;
					tileHeight1->elevation[0][y] = h;
				}
			}

			if (tileData0.first.y + 1 == tileData1.first.y && tileData0.first.x == tileData1.first.x)
			{
				for (int x = 0; x < tileHeight0->width; ++x)
				{
					float h0 = tileHeight0->elevation[x][tileHeight0->height - 1];
					float h1 = tileHeight1->elevation[x][0];
					float h = (h0 + h1) * 0.5f;
					tileHeight0->elevation[x][tileHeight0->height - 1] = h;
					tileHeight1->elevation[x][0] = h;
				}
			}
		}
	}
}*/
//-----------------------------------------------------------------------------
// Extract a plane geometry, vertices in [-1.0,1.0], for terrain mesh
//-----------------------------------------------------------------------------
PolygonMesh* CreateTerrainMesh(const Tile& tile, const HeightData* heightMap, const int terrainSubdivision)
{
	if (!heightMap)
		return NULL;

	PolygonMesh* mesh = new PolygonMesh(eLayerTerrain, NULL);
	buildPlane(mesh->vertices, mesh->indices, 2.0, 2.0, terrainSubdivision, terrainSubdivision);

	// Build terrain mesh extrusion, with bilinear height sampling
	for (auto& vertex : mesh->vertices)
	{
		v2 tilePosition = v2(vertex.position.x, vertex.position.y);
		vertex.position.z = sampleElevation(tilePosition, heightMap);
	}

	return mesh;
}
//-----------------------------------------------------------------------------
void CreatePedestalMeshes(const Tile& tile, const HeightData* heightMap, PolygonMesh* meshes[2], const int terrainSubdivision, float pedestalHeight)
{
	PolygonMesh* ground = new PolygonMesh(eLayerTerrain, NULL);
	PolygonMesh* wall = new PolygonMesh(eLayerTerrain, NULL);

	buildPlane(ground->vertices, ground->indices, 2.0, 2.0, terrainSubdivision, terrainSubdivision, true);

	for (auto& vertex : ground->vertices)
		vertex.position.z = pedestalHeight;

	buildPedestalPlanes(tile, wall->vertices, wall->indices, heightMap, terrainSubdivision, pedestalHeight);

	meshes[0] = ground;
	meshes[1] = wall;
}
//-----------------------------------------------------------------------------
// Split mesh into meshes for 16-bit indexing
//-----------------------------------------------------------------------------
static inline int SplitMesh(const PolygonMesh* src, std::vector<PolygonMesh*>& dstArr)
{
	const int src_nv = src->vertices.size();
	
	// No splitting needed
	if (src_nv <= 65536)
		return 0;

	int* remap = new int[src_nv];

	int i = 0;
	while (i < src->indices.size())
	{
		//PolygonMesh* dst = new PolygonMesh(src->layerType, src->feature);
		PolygonMesh* dst = new PolygonMesh(src->layerType, NULL);
		dst->feature_kind = src->feature_kind;
		dst->feature_name = src->feature_name;
		dst->feature_sortrank = src->feature_sortrank;

		dstArr.push_back(dst);

		for (int k = 0; k < src_nv; k++)
			remap[k] = -1;

		while (i < src->indices.size() && dst->vertices.size() <= (65536 - 3))
		{
			for (int k = 0; k < 3; k++)
			{
				const uint32 old_idx = src->indices[i + k];
				int new_idx = remap[old_idx];

				if (new_idx == -1)
				{
					new_idx = dst->vertices.size();
					remap[old_idx] = new_idx;
					dst->vertices.push_back(src->vertices[old_idx]);
				}

				dst->indices.push_back(new_idx);
			}

			i += 3;
		}
	}

	LOG("Split mesh: v: %d, t:%d into:\n", src->vertices.size(), src->indices.size()/3);
	for (size_t i = 0; i < dstArr.size(); i++)
	{
		LOG("%d: v: %d, t: %d\n", i, dstArr[i]->vertices.size(), dstArr[i]->indices.size()/3);
	}
	
	delete[] remap;

	return dstArr.size();
}
//-----------------------------------------------------------------------------
static inline void AddNewMesh(PolygonMesh* mesh, std::vector<PolygonMesh*>& tmpSplitArr, std::vector<PolygonMesh*>& outmeshes)
{
	int numSplits = SplitMesh(mesh, tmpSplitArr);
	if (numSplits == 0)
	{
		outmeshes.push_back(mesh);
	}
	else
	{
		delete mesh;
		outmeshes.insert(outmeshes.end(), tmpSplitArr.begin(), tmpSplitArr.end());
		tmpSplitArr.clear();
	}
}
//-----------------------------------------------------------------------------
CStrL TileFileName(const Vec3i& tms)
{
	return GetWriteFile(Str_Printf("cache/%d_%d_%d.bin", tms.x, tms.y, tms.z));
}
//-----------------------------------------------------------------------------
Vec3i maxTile;
//-----------------------------------------------------------------------------
struct Params2
{
	const char* apiKey;
	Vec3i		tms;
	bool		terrain;
	int			terrainSubdivision;
	float		terrainExtrusionScale;
	bool		vectorData;
};
//-----------------------------------------------------------------------------
static bool vectiler(const Params2& params, std::vector<PolygonMesh*>& meshes)
{
	HeightData* heightMap = NULL;
	Tile tile(params.tms.x, params.tms.y, params.tms.z);

	if (params.terrain)
	{
		heightMap = DownloadHeightmapTile(tile, params.apiKey, params.terrainExtrusionScale);

		if (!heightMap)
		{
			LOG("Failed to download heightmap texture data for tile %d %d %d\n", tile.x, tile.y, tile.z);
			return false;
		}
		/// Adjust terrain edges
		//if (exportParams.terrain)
		//	adjustTerrainEdges(heightData);
	}

	// Vector tile
	TArray<Layer> vectorLayers;
	if (params.vectorData)
	{
		if(!DownloadVectorTile(tile, params.apiKey, vectorLayers))
			LOG("Failed to download vector tile data for tile %d %d %d\n", tile.x, tile.y, tile.z);
	}

	// Build meshes for the tile
	std::vector<PolygonMesh*> tmpSplitArr;
	CStopWatch sw;
	
	// Build terrain mesh
	if (params.terrain)
	{
		PolygonMesh* mesh = CreateTerrainMesh(tile, heightMap, params.terrainSubdivision);
		computeNormals(mesh);	// Compute faces normals
		AddNewMesh(mesh, tmpSplitArr, meshes);
		/// Build pedestal
		/*if (params.pedestal)
		{
			PolygonMesh* groundWall[2];
			CreatePedestalMeshes(tile, heightMap, groundWall, exportParams.terrainSubdivision, exportParams.pedestalHeight);
			groundWall[0]->offset = offset;
			groundWall[1]->offset = offset;
			meshes.push_back(groundWall[0]);
			meshes.push_back(groundWall[1]);
		}*/
	}

	// Build vector tile meshes
	PolyMeshBuilder ctx;
	for (int l = 0; l < vectorLayers.Num(); l++)
	{
		const Layer& layer = vectorLayers[l];
		const ELayerType type = layer.layerType;

		if (heightMap && type != eLayerBuildings && type != eLayerRoads)	continue;

		for (int i = 0; i < layer.features.Num(); i++)
		{
			const Feature* feature = &layer.features[i];
			if (heightMap && (type != eLayerRoads) && (feature->height == 0.0f))	continue;
			
			PolygonMesh* mesh = CreatePolygonMeshFromFeature(type, feature, heightMap, ctx);
			if (mesh)
				AddNewMesh(mesh, tmpSplitArr, meshes);
		}
	}

	float t_BuildMeshes = sw.GetMs(true);

	static float maxt = 0;
	if (t_BuildMeshes > maxt)
	{
		maxt = t_BuildMeshes;
		maxTile.x = tile.x;
		maxTile.y = tile.y;
		maxTile.z = tile.z;
	}

	LOG("Triangulated %d PolygonMeshes in %.1fms\n", meshes.size(), t_BuildMeshes);

	return true;
}
float t_disc = 0;
//-----------------------------------------------------------------------------
bool GetTile(const Vec3i& tms, TArray<StreamGeom*>& geoms)
{
	const CStrL fname = TileFileName(tms);
	//Vec2i google = TmsToGoogleTile(Vec2i(tms.x, tms.y), tms.z);

	// See if tile is binary cached
	if (tile_DiscCache == eDiscBinaryCache)
	{
		CStopWatch sw;
		if (LoadBin(fname, geoms))
		{
			t_disc += sw.GetMs();
			return true;
		}
	}
	
	// Mapzen uses google xyz indexing
	struct Params2 params =
	{
		"vector-tiles-qVaBcRA",	// apiKey
		tms,					// tile x,y,z
		false,					// terrain. Generate terrain elevation topography
		64,						// terrainSubdivision
		1.0f,					// terrainExtrusionScale
		true					// vectorData. Buildings, roads, landuse, pois, etc...
	};

	// Download & triangulate tile
	std::vector<PolygonMesh*> meshes;
	if (!vectiler(params, meshes))
		return false;

	// Save tile
	SaveBin(fname, meshes);

	// Delete all meshes
	for (size_t i = 0; i < meshes.size(); i++)
		delete meshes[i];

	if (!LoadBin(fname, geoms))
		return false;

	return true;
}
