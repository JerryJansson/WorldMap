#include "Precompiled.h"
#include <unordered_map>
#include "tiledata.h"
#include "vectiler.h"
#include "download.h"
#include "geometry.h"

//-----------------------------------------------------------------------------
void buildPlane(std::vector<PolygonVertex>& outVertices,
	std::vector<unsigned int>& outIndices,
	float width,       // Total plane width (x-axis)
	float height,      // Total plane height (y-axis)
	unsigned int nw,   // Split on width
	unsigned int nh,   // Split on height
	bool flip = false)
{
	// TODO: add offsets
	std::vector<glm::vec4> vertices;
	std::vector<int> indices;

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

			outVertices.push_back({ v0, normal });
			outVertices.push_back({ v1, normal });
			outVertices.push_back({ v2, normal });
			outVertices.push_back({ v3, normal });

			if (!flip) {
				outIndices.push_back(indexOffset + 0);
				outIndices.push_back(indexOffset + 1);
				outIndices.push_back(indexOffset + 2);
				outIndices.push_back(indexOffset + 0);
				outIndices.push_back(indexOffset + 2);
				outIndices.push_back(indexOffset + 3);
			}
			else {
				outIndices.push_back(indexOffset + 0);
				outIndices.push_back(indexOffset + 2);
				outIndices.push_back(indexOffset + 1);
				outIndices.push_back(indexOffset + 0);
				outIndices.push_back(indexOffset + 3);
				outIndices.push_back(indexOffset + 2);
			}

			indexOffset += 4;
		}
	}
}

//-----------------------------------------------------------------------------
void buildPedestalPlanes(const Tile& tile,
	std::vector<PolygonVertex>& outVertices,
	std::vector<unsigned int>& outIndices,
	const HeightData* elevation,
	unsigned int subdiv,
	float pedestalHeight)
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

			normalVector = glm::cross(upVector, v0 - v1);
			normalVector = glm::normalize(normalVector);

			float h0 = sampleElevation(v2(v0.x, v0.y), elevation);
			float h1 = sampleElevation(v2(v1.x, v1.y), elevation);

			v0.z = h0 * tile.invScale;
			outVertices.push_back({ v0, normalVector });
			v1.z = h1 * tile.invScale;
			outVertices.push_back({ v1, normalVector });
			v0.z = pedestalHeight * tile.invScale;
			outVertices.push_back({ v0, normalVector });
			v1.z = pedestalHeight * tile.invScale;
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
void adjustTerrainEdges(std::unordered_map<Tile, HeightData*>& heightData)
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
}
//-----------------------------------------------------------------------------
// Extract a plane geometry, vertices in [-1.0,1.0], for terrain mesh
//-----------------------------------------------------------------------------
PolygonMesh* CreateTerrainMesh(const Tile& tile, const HeightData* heightMap, const int terrainSubdivision)
{
	if (!heightMap)
		return NULL;

	PolygonMesh* mesh = new PolygonMesh(eLayerTerrain);
	buildPlane(mesh->vertices, mesh->indices, 2.0, 2.0, terrainSubdivision, terrainSubdivision);

	// Build terrain mesh extrusion, with bilinear height sampling
	for (auto& vertex : mesh->vertices)
	{
		v2 tilePosition = v2(vertex.position.x, vertex.position.y);
		float extrusion = sampleElevation(tilePosition, heightMap);
		vertex.position.z = extrusion * tile.invScale;				// Scale the height within the tile scale
	}

	return mesh;
}
//-----------------------------------------------------------------------------
void CreatePedestalMeshes(const Tile& tile, const HeightData* heightMap, PolygonMesh* meshes[2], const int terrainSubdivision, float pedestalHeight)
{
	PolygonMesh* ground = new PolygonMesh(eLayerTerrain);
	PolygonMesh* wall = new PolygonMesh(eLayerTerrain);

	buildPlane(ground->vertices, ground->indices, 2.0, 2.0, terrainSubdivision, terrainSubdivision, true);

	for (auto& vertex : ground->vertices)
		vertex.position.z = pedestalHeight * tile.invScale;

	buildPedestalPlanes(tile, wall->vertices, wall->indices, heightMap, terrainSubdivision, pedestalHeight);

	meshes[0] = ground;
	meshes[1] = wall;
}
//-----------------------------------------------------------------------------
// Separate all meshes in it's respective layer
// Merge all meshes from same layer into 1 big mesh. If this mesh gets vcount>65536 the mesh is split
//-----------------------------------------------------------------------------
void MergeLayerMeshes(const std::vector<PolygonMesh*>& meshes, std::vector<PolygonMesh*> meshArr[eNumLayerTypes])
{
	for (PolygonMesh* mesh : meshes)
	{
		// This mesh is to big. Throw it away
		if (mesh->vertices.size() > 65536) {
			gLogger.Warning("Mesh > 65536. Must implement mesh splitting");
			continue;
		}

		std::vector<PolygonMesh*>& arr = meshArr[mesh->layerType];	// Choose correct layer
		PolygonMesh* bigMesh = arr.empty() ? NULL : arr.back();
		if (!bigMesh || !AddMeshToMesh(mesh, bigMesh))				// Try to add current mesh to our bigMesh
		{
			bigMesh = new PolygonMesh(mesh->layerType);
			arr.push_back(bigMesh);
			AddMeshToMesh(mesh, bigMesh);
		}
	}

	/*for (int i = 0; i < eNumLayerTypes; i++)
	{
		if(meshArr[i].size())
			LOG("%s: %d meshes\n", layerNames[i], meshArr[i].size());
	}*/
}
//-----------------------------------------------------------------------------
bool vectiler2(const Params2 params)
{
	HeightData* heightMap = NULL;
	TileVectorData* vectorTileData = NULL;
	Tile tile(params.tilex, params.tiley, params.tilez);

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

	if (params.buildings || params.roads)
	{
		vectorTileData = DownloadVectorTile(tile, params.apiKey);

		if (!vectorTileData)
			LOG("Failed to download vector tile data for tile %d %d %d\n", tile.x, tile.y, tile.z);
	}

	std::vector<PolygonMesh*> meshes;

	// Build meshes for the tile
	CStopWatch sw;
	
	// Build terrain mesh
	if (params.terrain)
	{
		PolygonMesh* mesh = CreateTerrainMesh(tile, heightMap, params.terrainSubdivision);
		computeNormals(mesh);	// Compute faces normals
		meshes.push_back(mesh);

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

	/// Build vector tile meshes
	if (vectorTileData)
	{
		const TileVectorData* data = vectorTileData;
		//const static std::string keyHeight("height");
		//const static std::string keyMinHeight("min_height");
		const float scale = tile.invScale * params.buildingsExtrusionScale;

		for (auto layer : data->layers)
		{
			//const int typeIdx = IndexFromStringTable(layer.name.c_str(), layerNames);
			//const ELayerType type = typeIdx >= 0 ? (ELayerType)typeIdx : eLayerUnknown;
			const ELayerType type = layer.layerType;
			if (type == eLayerBuildings && !params.buildings) continue;	// Skip buildings
			if (type == eLayerRoads && !params.roads) continue;			// Skip roads

			const float default_height = type == eLayerBuildings ? params.buildingsHeight * tile.invScale : 0.0f;

			for (auto feature : layer.features)
			{
				if (heightMap && type != eLayerBuildings && type != eLayerRoads) continue;

				// Height
				const float height = feature.height > 0 ? feature.height : default_height;
				/*float height = default_height;
				auto itHeight = feature.props.numericProps.find(keyHeight);
				if (itHeight != feature.props.numericProps.end())
					height = itHeight->second * scale;*/

				if (heightMap && (type != eLayerRoads) && (height == 0.0f))
					continue;

				// Min height
				const float minHeight = feature.min_height > 0 ? feature.min_height : default_height;
				/*double minHeight = 0.0;
				auto itMinHeight = feature.props.numericProps.find(keyMinHeight);
				if (itMinHeight != feature.props.numericProps.end())
					minHeight = itMinHeight->second * scale;*/

				auto mesh = CreateMeshFromFeature(type, feature, minHeight, height, heightMap, tile.invScale, params.roadsHeight);
				if (mesh)
					meshes.push_back(mesh);
			}
		}
	}

	float t_BuildMeshes = sw.GetMs(true);
	// Separate all meshes in it's respective layer
	// Merge all meshes from same layer into 1 big mesh. If this mesh gets vcount>65536 the mesh is split
	std::vector<PolygonMesh*> meshArr[eNumLayerTypes];
	MergeLayerMeshes(meshes, meshArr);
	float t_MergeMeshes = sw.GetMs();

	LOG("Built %d layermeshes (%.1fms). Merged meshes (%.1fms)\n", meshes.size(), t_BuildMeshes, t_MergeMeshes);

	// Save output BIN file
	CStrL fname = Str_Printf("%d_%d_%d.bin", tile.x, tile.y, tile.z);
	SaveBin(fname, meshArr);

	/*std::string outFile = std::to_string(origin.x) + "." + std::to_string(origin.y) + "." + std::to_string(origin.z);
	std::string outputOBJ = outFile + ".obj";
	
	// Save output OBJ file
	bool saved = saveOBJ(outputOBJ.c_str(),
		exportParams.splitMesh, meshes,
		exportParams.offset[0],
		exportParams.offset[1],
		exportParams.append,
		exportParams.normals);

	if (!saved)
		return EXIT_FAILURE;*/

	return true;
}