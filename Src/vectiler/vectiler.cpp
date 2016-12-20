#include "Precompiled.h"
#include <fstream>
#include <unordered_map>
#include "tiledata.h"
#include "vectiler.h"
#include "download.h"
#include "geometry.h"

#ifdef AOBAKER
#include "aobaker.h"
#else
int aobaker_bake(
	char const* inputmesh,
	char const* outputmesh,
	char const* outputatlas,
	int sizehint,
	int nsamples,
	bool gbuffer,
	bool chartinfo,
	float multiply)
{
	printf("WARNING -- Vectiler was built without ao baker\n");
	return 0;
}
#endif

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
bool extractTileRange(const CStr& range, int& start, int& end)
{
	CStr tmp;
	int p = 0;

	if (!range.Tokenize('/', p, tmp))
		return false;

	start = tmp.ToInt();
	if (range.Tokenize('/', p, tmp))	end = tmp.ToInt();
	else								end = start;

	return end >= start;
}
//-----------------------------------------------------------------------------
bool ExtractTiles(const char* tilex, const char* tiley, const int tilez, std::vector<Tile>& tiles)
{
	int startx, starty, endx, endy;
	if (!extractTileRange(CStr(tilex), startx, endx)) { LOG("Bad param: %s\n", tilex); return false; }
	if (!extractTileRange(CStr(tiley), starty, endy)) { LOG("Bad param: %s\n", tiley); return false; }

	for (int x = startx; x <= endx; ++x)
	{
		for (int y = starty; y <= endy; ++y)
		{
			Tile t(x, y, tilez);

			if (x == startx)	t.borders.set(Border::left, 1);
			if (x == endx)		t.borders.set(Border::right, 1);
			if (y == starty)	t.borders.set(Border::top, 1);
			if (y == endy)      t.borders.set(Border::bottom, 1);

			tiles.push_back(t);
		}
	}

	if (tiles.size() == 0) { LOG("No tiles to download\n"); return false; }

	return true;
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
	CStopWatch sw;
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

	LOG("Merged (%.1fms) %d meshes into:\n", sw.GetMs(), meshes.size());
	for (int i = 0; i < eNumLayerTypes; i++)
	{
		LOG("%s: %d meshes\n", layerNames[i], meshArr[i].size());
	}
}
//-----------------------------------------------------------------------------
int vectiler(Params exportParams, CStrL& outFileName)
{
	/// Parse tile params
	std::vector<Tile> tiles;
	if (!ExtractTiles(exportParams.tilex, exportParams.tiley, exportParams.tilez, tiles))
		return EXIT_FAILURE;

	const char* apiKey = exportParams.apiKey;
	LOG("Using API key %s\n", apiKey);

	//std::unordered_map<Tile, std::unique_ptr<HeightData>> heightData;
	//std::unordered_map<Tile, std::unique_ptr<TileData>> vectorTileData;
	std::unordered_map<Tile, HeightData*> heightData;
	std::unordered_map<Tile, TileVectorData*> vectorTileData;

	/// Download data
	LOG("---- Downloading tile data ----\n");

	for (auto tile : tiles)
	{
		if (exportParams.terrain)
		{
			auto textureData = DownloadHeightmapTile(tile, apiKey, exportParams.terrainExtrusionScale);

			if (!textureData)
				LOG("Failed to download heightmap texture data for tile %d %d %d\n", tile.x, tile.y, tile.z);

			heightData[tile] = std::move(textureData);
		}

		if (exportParams.buildings || exportParams.roads)
		{
			auto tileData = DownloadVectorTile(tile, apiKey);

			if (!tileData)
				LOG("Failed to download vector tile data for tile %d %d %d\n", tile.x, tile.y, tile.z);

			vectorTileData[tile] = std::move(tileData);
		}
	}

	/// Adjust terrain edges
	if (exportParams.terrain)
		adjustTerrainEdges(heightData);

	//std::vector<std::unique_ptr<PolygonMesh>> meshes;
	std::vector<PolygonMesh*> meshes;

	v2 offset;
	Tile origin = tiles[0];

	LOG("---- Building tile data ----\n");
	CStopWatch sw;
	// Build meshes for each of the tiles
	for (auto tile : tiles)
	{
		offset.x = (tile.x - origin.x) * 2;
		offset.y = -(tile.y - origin.y) * 2;

		//const auto& textureData = heightData[tile];
		const auto heightMap = heightData[tile];

		/// Build terrain mesh
		if (exportParams.terrain)
		{
			if (!heightMap)
				continue;
			
			PolygonMesh* mesh = CreateTerrainMesh(tile, heightMap, exportParams.terrainSubdivision);
			if (exportParams.normals) computeNormals(mesh);	// Compute faces normals
			mesh->offset = offset;
			meshes.push_back(mesh);

			/// Build pedestal
			if (exportParams.pedestal)
			{
				PolygonMesh* groundWall[2];
				CreatePedestalMeshes(tile, heightMap, groundWall, exportParams.terrainSubdivision, exportParams.pedestalHeight);
				groundWall[0]->offset = offset;
				groundWall[1]->offset = offset;
				meshes.push_back(groundWall[0]);
				meshes.push_back(groundWall[1]);
			}
		}

		/// Build vector tile mesh
		if (exportParams.buildings || exportParams.roads)
		{
			const auto& data = vectorTileData[tile];

			if (data)
			{
				const static std::string keyHeight("height");
				const static std::string keyMinHeight("min_height");
				const float scale = tile.invScale * exportParams.buildingsExtrusionScale;

				for (auto layer : data->layers)
				{
					ELayerType type = eLayerOther;
					if (layer.name == "water")			type = eLayerWater;
					else if (layer.name == "buildings") type = eLayerBuildings;
					else if (layer.name == "roads")		type = eLayerRoads;
					
					if (type == eLayerBuildings && !exportParams.buildings) continue;	// Skip buildings
					if (type == eLayerRoads && !exportParams.roads) continue;			// Skip roads

					const float default_height = type == eLayerBuildings ? exportParams.buildingsHeight * tile.invScale : 0.0f;

					for (auto feature : layer.features)
					{
						//if (textureData && !layerBuildings && !layerRoads) continue;
						if (heightMap && type != eLayerBuildings && type != eLayerRoads) continue;

						// Height
						float height = default_height;
						auto itHeight = feature.props.numericProps.find(keyHeight);
						if (itHeight != feature.props.numericProps.end())
							height = itHeight->second * scale;

						//if (textureData && !layerRoads && height == 0.0f)
						if (heightMap && type != eLayerRoads && height == 0.0f)
							continue;

						// Min height
						double minHeight = 0.0;
						auto itMinHeight = feature.props.numericProps.find(keyMinHeight);
						if (itMinHeight != feature.props.numericProps.end())
							minHeight = itMinHeight->second * scale;

						// JJ 
						//auto mesh = new PolygonMesh(type);
						auto mesh = CreateMeshFromFeature(type, feature, minHeight, height, heightMap, tile.invScale, exportParams.roadsExtrusionWidth, exportParams.roadsHeight);

						// Add local mesh offset
						mesh->offset = offset;
						meshes.push_back(mesh);
					}
				}
			}
		}
	}

	LOG("Built PolygonMeshes in %.1fms\n", sw.GetMs());

	// Separate all meshes in it's respective layer
	std::vector<PolygonMesh*> meshArr[eNumLayerTypes];
	MergeLayerMeshes(meshes, meshArr);

	// Merge all meshes from same layer into 1 big mesh. If this mesh gets vcount>65536 the mesh is split
	/*for (PolygonMesh* mesh : meshes)
	{
		const int nv = mesh->vertices.size();

		// This mesh is to big. Throw it away
		if (nv > 65536)
		{
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
	}*/


	std::string outFile = std::to_string(origin.x) + "." + std::to_string(origin.y) + "." + std::to_string(origin.z);
	std::string outputOBJ = outFile + ".obj";
	std::string outputBIN = outFile + ".bin";
	outFileName = outputBIN.c_str();

	// Save output OBJ file
	bool saved = saveOBJ(outputOBJ.c_str(),
		exportParams.splitMesh, meshes,
		exportParams.offset[0],
		exportParams.offset[1],
		exportParams.append,
		exportParams.normals);

	// Save output BIN file
	SaveBin(outputBIN.c_str(), meshArr);

	//bool saved2 = SaveOBJ2("jerry.obj", meshArr);

	if (!saved)
		return EXIT_FAILURE;

	// Bake ambiant occlusion using AO Baker
	if (exportParams.aoBaking)
	{
		std::string aoFile = outFile + "-ao.obj";
		std::string aoImageFile = outFile + ".png";
		std::string aoMaterialFile = outFile + ".mtl";

		int aoBaked = aobaker_bake(outputOBJ.c_str(), aoFile.c_str(), aoImageFile.c_str(),
			exportParams.aoSizeHint, exportParams.aoSamples,
			false,  // g-buffers
			false,  // charinfo
			1.0f);   // multiply

		bool success = aoBaked == 1;
		std::string aoFileData;

		{
			std::ifstream ifile(aoFile);
			success &= ifile.is_open();

			if (success) {
				aoFileData = std::string((std::istreambuf_iterator<char>(ifile)), (std::istreambuf_iterator<char>()));
				ifile.close();
			}
		}

		// Inject material to wavefront
		{
			std::ofstream ofile(aoFile);
			success &= ofile.is_open();

			if (success) {
				std::string materialHeader;
				materialHeader += "mtllib " + aoMaterialFile + "\n";
				materialHeader += "usemtl tile_mtl\n\n";
				ofile << materialHeader;
				ofile << aoFileData;
				ofile.close();
			}
		}

		// Export material properties
		{
			std::ofstream materialfile(aoMaterialFile);
			success &= materialfile.is_open();

			if (success) {
				printf("Saving material file %s\n", aoMaterialFile.c_str());
				std::string material = R"END(
                    newmtl tile_mtl
                    Ka 0.0000 0.0000 0.0000
                    Kd 1.0000 1.0000 1.0000
                    Ks 0.0000 0.0000 0.0000
                    d 1.0
                    map_Kd )END";
				material += aoImageFile;
				materialfile << material;
				materialfile.close();
			}
		}

		if (!success)
			return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

#if 1
//-----------------------------------------------------------------------------
bool vectiler2(Params2 params)
{
	LOG("Using API key %s\n", params.apiKey);

	HeightData* heightMap = NULL;
	TileVectorData* vectorTileData = NULL;

	v2 offset(0.0f, 0.0f);
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

	//v2 offset;
	//Tile origin = tiles[0];

	LOG("---- Building tile data ----\n");
	CStopWatch sw;
	// Build meshes for the tile
	//offset.x = (tile.x - origin.x) * 2;
	//offset.y = -(tile.y - origin.y) * 2;

	// Build terrain mesh
	if (params.terrain)
	{
		PolygonMesh* mesh = CreateTerrainMesh(tile, heightMap, params.terrainSubdivision);
		computeNormals(mesh);	// Compute faces normals
		mesh->offset = offset;
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

	/// Build vector tile mesh
	if (vectorTileData && (params.buildings || params.roads))
	{
		const TileVectorData* data = vectorTileData;
		const static std::string keyHeight("height");
		const static std::string keyMinHeight("min_height");
		const float scale = tile.invScale * params.buildingsExtrusionScale;

		for (auto layer : data->layers)
		{
			ELayerType type = eLayerOther;
			if (layer.name == "water")			type = eLayerWater;
			else if (layer.name == "buildings") type = eLayerBuildings;
			else if (layer.name == "roads")		type = eLayerRoads;

			if (type == eLayerBuildings && !params.buildings) continue;	// Skip buildings
			if (type == eLayerRoads && !params.roads) continue;			// Skip roads

			const float default_height = type == eLayerBuildings ? params.buildingsHeight * tile.invScale : 0.0f;

			for (auto feature : layer.features)
			{
				if (heightMap && type != eLayerBuildings && type != eLayerRoads) continue;

				// Height
				float height = default_height;
				auto itHeight = feature.props.numericProps.find(keyHeight);
				if (itHeight != feature.props.numericProps.end())
					height = itHeight->second * scale;

				if (heightMap && (type != eLayerRoads) && (height == 0.0f))
					continue;

				// Min height
				double minHeight = 0.0;
				auto itMinHeight = feature.props.numericProps.find(keyMinHeight);
				if (itMinHeight != feature.props.numericProps.end())
					minHeight = itMinHeight->second * scale;

				auto mesh = CreateMeshFromFeature(type, feature, minHeight, height, heightMap, tile.invScale, params.roadsExtrusionWidth, params.roadsHeight);

				// Add local mesh offset
				mesh->offset = offset;
				meshes.push_back(mesh);
			}
		}
	}

	LOG("Built PolygonMeshes from layers in %.1fms\n", sw.GetMs(true));

	// Separate all meshes in it's respective layer
	// Merge all meshes from same layer into 1 big mesh. If this mesh gets vcount>65536 the mesh is split
	std::vector<PolygonMesh*> meshArr[eNumLayerTypes];
	MergeLayerMeshes(meshes, meshArr);

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
#endif