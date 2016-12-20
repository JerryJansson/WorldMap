#include "Precompiled.h"
#include <fstream>
#include <unordered_map>
#include <memory>

#include "rapidjson/document.h"

#include "geojson.h"
#include "tiledata.h"
#include "vectiler.h"
#include "earcut.h"

#include "download.h"
#include "geometry.h"

//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

#define EPSILON 1e-5f
//-----------------------------------------------------------------------------
struct HeightData {
    std::vector<std::vector<float>> elevation;
    int width;
    int height;
};
//-----------------------------------------------------------------------------
bool withinTileRange(const v2& pos) { return pos.x >= -1.0 && pos.x <= 1.0 && pos.y >= -1.0 && pos.y <= 1.0; }
/*-----------------------------------------------------------------------------
 * Sample elevation using bilinear texture sampling
 * - position: must lie within tile range [-1.0, 1.0]
 * - textureData: the elevation tile data, may be null
 //-----------------------------------------------------------------------------*/
float sampleElevation(v2 position, const std::unique_ptr<HeightData>& textureData)
{
    if (!textureData)
		return 0.0;

    if (!withinTileRange(position))
        position = myClamp(position, v2(-1.0), v2(1.0));

    // Normalize vertex coordinates into the texture coordinates range
    float u = (position.x * 0.5f + 0.5f) * textureData->width;
    float v = (position.y * 0.5f + 0.5f) * textureData->height;

    // Flip v coordinate according to tile coordinates
    v = textureData->height - v;

    float alpha = u - floor(u);
    float beta  = v - floor(v);

    int ii0 = floor(u);
    int jj0 = floor(v);
    int ii1 = ii0 + 1;
    int jj1 = jj0 + 1;

    // Clamp on borders
    ii0 = std::min(ii0, textureData->width - 1);
    jj0 = std::min(jj0, textureData->height -1);
    ii1 = std::min(ii1, textureData->width - 1);
    jj1 = std::min(jj1, textureData->height - 1);

    // Sample four corners of the current texel
    float s0 = textureData->elevation[ii0][jj0];
    float s1 = textureData->elevation[ii0][jj1];
    float s2 = textureData->elevation[ii1][jj0];
    float s3 = textureData->elevation[ii1][jj1];

    // Sample the bilinear height from the elevation texture
    float bilinearHeight = (1 - beta) * (1 - alpha) * s0
                         + (1 - beta) * alpha       * s1
                         + beta       * (1 - alpha) * s2
                         + alpha      * beta        * s3;

    return bilinearHeight;
}
//-----------------------------------------------------------------------------
v2 centroid(const std::vector<std::vector<v3>>& polygon)
{
    v2 centroid(0,0);
    int n = 0;

    for (auto& l : polygon)
	{
        for (auto& p : l)
		{
            centroid.x += p.x;
            centroid.y += p.y;
            n++;
        }
    }

    if (n > 0)
        centroid /= n;
    
	return centroid;
}
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

            outVertices.push_back({v0, normal});
            outVertices.push_back({v1, normal});
            outVertices.push_back({v2, normal});
            outVertices.push_back({v3, normal});

            if (!flip) {
                outIndices.push_back(indexOffset+0);
                outIndices.push_back(indexOffset+1);
                outIndices.push_back(indexOffset+2);
                outIndices.push_back(indexOffset+0);
                outIndices.push_back(indexOffset+2);
                outIndices.push_back(indexOffset+3);
            } else {
                outIndices.push_back(indexOffset+0);
                outIndices.push_back(indexOffset+2);
                outIndices.push_back(indexOffset+1);
                outIndices.push_back(indexOffset+0);
                outIndices.push_back(indexOffset+3);
                outIndices.push_back(indexOffset+2);
            }

            indexOffset += 4;
        }
    }
}
//-----------------------------------------------------------------------------
float buildPolygonExtrusion(const Polygon2& polygon,
    double minHeight,
    double height,
    std::vector<PolygonVertex>& outVertices,
    std::vector<unsigned int>& outIndices,
    const std::unique_ptr<HeightData>& elevation,
    float inverseTileScale)
{
    int vertexDataOffset = outVertices.size();
    v3 upVector(0.0f, 0.0f, 1.0f);
    v3 normalVector;
    float minz = 0.f;
    float cz = 0.f;

    // Compute min and max height of the polygon
    if (elevation) {
        // The polygon centroid height
        cz = sampleElevation(centroid(polygon), elevation);
        minz = std::numeric_limits<float>::max();

        for (auto& line : polygon) {
            for (size_t i = 0; i < line.size(); i++) {
                v3 p(line[i]);

                float pz = sampleElevation(v2(p.x, p.y), elevation);

                minz = std::min(minz, pz);
            }
        }
    }

    for (auto& line : polygon) {
        size_t lineSize = line.size();

        outVertices.reserve(outVertices.size() + lineSize * 4);
        outIndices.reserve(outIndices.size() + lineSize * 6);

        for (size_t i = 0; i < lineSize - 1; i++) {
            v3 a(line[i]);
            v3 b(line[i+1]);

            if (a == b) { continue; }

            normalVector = glm::cross(upVector, b - a);
            normalVector = glm::normalize(normalVector);

            a.z = height + cz * inverseTileScale;
            outVertices.push_back({a, normalVector});
            b.z = height + cz * inverseTileScale;
            outVertices.push_back({b, normalVector});
            a.z = minHeight + minz * inverseTileScale;
            outVertices.push_back({a, normalVector});
            b.z = minHeight + minz * inverseTileScale;
            outVertices.push_back({b, normalVector});

            outIndices.push_back(vertexDataOffset+0);
            outIndices.push_back(vertexDataOffset+1);
            outIndices.push_back(vertexDataOffset+2);
            outIndices.push_back(vertexDataOffset+1);
            outIndices.push_back(vertexDataOffset+3);
            outIndices.push_back(vertexDataOffset+2);

            vertexDataOffset += 4;
        }
    }

    return cz;
}
//-----------------------------------------------------------------------------
void buildPolygon(const Polygon2& polygon,
    double height,
    std::vector<PolygonVertex>& outVertices,
    std::vector<unsigned int>& outIndices,
    const std::unique_ptr<HeightData>& elevation,
    float centroidHeight,
    float inverseTileScale)
{
    mapbox::Earcut<float, unsigned int> earcut;
    earcut(polygon);
	if (earcut.indices.size() == 0)
		return;
	
	unsigned int vertexDataOffset = outVertices.size();

    if (vertexDataOffset == 0)
	{
        outIndices = std::move(earcut.indices);
    } 
	else
	{
        outIndices.reserve(outIndices.size() + earcut.indices.size());
        for (auto i : earcut.indices) 
		{
            outIndices.push_back(vertexDataOffset + i);
        }
    }

    static v3 normal(0.0, 0.0, 1.0);
    outVertices.reserve(outVertices.size() + earcut.vertices.size());
    centroidHeight *= inverseTileScale;

    for (auto& p : earcut.vertices)
	{
        v2 position(p[0], p[1]);
        v3 coord(position.x, position.y, height + centroidHeight);
        outVertices.push_back({coord, normal});
    }
}
//-----------------------------------------------------------------------------
void buildPedestalPlanes(const Tile& tile,
    std::vector<PolygonVertex>& outVertices,
    std::vector<unsigned int>& outIndices,
    const std::unique_ptr<HeightData>& elevation,
    unsigned int subdiv,
    float pedestalHeight)
{
    float offset = 1.0 / subdiv;
    int vertexDataOffset = outVertices.size();

    for (size_t i = 0; i < tile.borders.size(); ++i) {
        if (!tile.borders[i]) {
            continue;
        }

        for (float x = -1.0; x < 1.0; x += offset) {
            static const v3 upVector(0.0, 0.0, 1.0);
            v3 v0, v1;

            if (i == Border::right) {
                v0 = v3(1.0, x + offset, 0.0);
                v1 = v3(1.0, x, 0.0);
            }

            if (i == Border::left) {
                v0 = v3(-1.0, x + offset, 0.0);
                v1 = v3(-1.0, x, 0.0);
            }

            if (i == Border::top) {
                v0 = v3(x + offset, 1.0, 0.0);
                v1 = v3(x, 1.0, 0.0);
            }

            if (i == Border::bottom) {
                v0 = v3(x + offset, -1.0, 0.0);
                v1 = v3(x, -1.0, 0.0);
            }

            v3 normalVector;

            normalVector = glm::cross(upVector, v0 - v1);
            normalVector = glm::normalize(normalVector);

            float h0 = sampleElevation(v2(v0.x, v0.y), elevation);
            float h1 = sampleElevation(v2(v1.x, v1.y), elevation);

            v0.z = h0 * tile.invScale;
            outVertices.push_back({v0, normalVector});
            v1.z = h1 * tile.invScale;
            outVertices.push_back({v1, normalVector});
            v0.z = pedestalHeight * tile.invScale;
            outVertices.push_back({v0, normalVector});
            v1.z = pedestalHeight * tile.invScale;
            outVertices.push_back({v1, normalVector});

            if (i == Border::right || i == Border::bottom) {
                outIndices.push_back(vertexDataOffset+0);
                outIndices.push_back(vertexDataOffset+1);
                outIndices.push_back(vertexDataOffset+2);
                outIndices.push_back(vertexDataOffset+1);
                outIndices.push_back(vertexDataOffset+3);
                outIndices.push_back(vertexDataOffset+2);
            } else {
                outIndices.push_back(vertexDataOffset+0);
                outIndices.push_back(vertexDataOffset+2);
                outIndices.push_back(vertexDataOffset+1);
                outIndices.push_back(vertexDataOffset+1);
                outIndices.push_back(vertexDataOffset+2);
                outIndices.push_back(vertexDataOffset+3);
            }

            vertexDataOffset += 4;
        }
    }
}
//-----------------------------------------------------------------------------
v3 perp(const v3& v) { return glm::normalize(v3(-v.y, v.x, 0.0)); }
//-----------------------------------------------------------------------------
v3 computeMiterVector(const v3& d0,
    const v3& d1,
    const v3& n0,
    const v3& n1)
{
    v3 miter = glm::normalize(n0 + n1);
    float miterl2 = glm::dot(miter, miter);

    if (miterl2 < std::numeric_limits<float>::epsilon()) {
        miter = v3(n1.y - n0.y, n0.x - n1.x, 0.0);
    } else {
        float theta = atan2f(d1.y, d1.x) - atan2f(d0.y, d0.x);
        if (theta < 0.f) { theta += 2 * M_PI; }
        miter *= 1.f / std::max<float>(sin(theta * 0.5f), EPSILON);
    }

    return miter;
}
//-----------------------------------------------------------------------------
void addPolygonPolylinePoint(Line& line,
    const v3 curr,
    const v3 next,
    const v3 last,
    const float extrude,
    size_t lineDataSize,
    size_t i,
    bool forward)
{
    v3 n0 = perp(curr - last);
    v3 n1 = perp(next - curr);
    bool right = glm::cross(n1, n0).z > 0.0;

    if ((i == 1 && forward) || (i == lineDataSize - 2 && !forward)) {
        line.push_back(last + n0 * extrude);
        line.push_back(last - n0 * extrude);
    }

    if (right) {
        v3 d0 = glm::normalize(last - curr);
        v3 d1 = glm::normalize(next - curr);
        v3 miter = computeMiterVector(d0, d1, n0, n1);
        line.push_back(curr - miter * extrude);
    } else {
        line.push_back(curr - n0 * extrude);
        line.push_back(curr - n1 * extrude);
    }
}
//-----------------------------------------------------------------------------
void adjustTerrainEdges(std::unordered_map<Tile, std::unique_ptr<HeightData>>& heightData)
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
std::unique_ptr<HeightData> DownloadHeightmapTile(const Tile& tile, const char* apiKey, float extrusionScale)
{
	CStrL url = TerrainURL(tile.x, tile.y, tile.z, apiKey);
	MemoryStruct out;
	if (!DownloadData(out, url))
		return nullptr;

	// Decode texture PNG
	int width, height, comp;
	unsigned char* pixels = stbi_load_from_memory((stbi_uc*)out.memory, out.size, &width, &height, &comp, STBI_rgb_alpha);

	if (comp != STBI_rgb_alpha)
	{
		printf("Failed to decompress PNG file\n");
		return nullptr;
	}

	std::unique_ptr<HeightData> data = std::unique_ptr<HeightData>(new HeightData());

	data->elevation.resize(height);
	for (int i = 0; i < height; ++i)
		data->elevation[i].resize(width);

	data->width = width;
	data->height = height;

	unsigned char* pixel = pixels;
	for (int i = 0; i < width * height; i++, pixel += 4)
	{
		float red = *(pixel + 0);
		float green = *(pixel + 1);
		float blue = *(pixel + 2);

		// Decode the elevation packed data from color component
		float elevation = (red * 256 + green + blue / 256) - 32768;

		int y = i / height;
		int x = i % width;

		assert(x >= 0 && x <= width && y >= 0 && y <= height);

		data->elevation[x][y] = elevation * extrusionScale;
	}

	return data;
}
//-----------------------------------------------------------------------------
std::unique_ptr<TileData> DownloadTile(const Tile& tile, const char* apiKey)
{
	CStrL url = VectorTileURL(tile.x, tile.y, tile.z, apiKey);
	MemoryStruct out;
	if (!DownloadData(out, url))
		return nullptr;

	// Parse written data into a JSON object
	CStopWatch sw;
	rapidjson::Document doc;
	doc.Parse(out.memory);

	if (doc.HasParseError())
	{
		LOG("Error parsing tile\n");
		rapidjson::ParseErrorCode code = doc.GetParseError();
		size_t errOffset = doc.GetErrorOffset();
		return nullptr;
	}

	float tJson = sw.GetMs(true);

	std::unique_ptr<TileData> data = std::unique_ptr<TileData>(new TileData());
	for (auto layer = doc.MemberBegin(); layer != doc.MemberEnd(); ++layer)
	{
		data->layers.emplace_back(std::string(layer->name.GetString()));
		GeoJson::extractLayer(layer->value, data->layers.back(), tile);
	}

	float tLayers = sw.GetMs();

	LOG("Parsed json in %.1fms. Built GeoJson structures in %.1fms\n", tJson, tLayers);

	return data;
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
int vectiler(Params exportParams, CStrL& outFileName)
{
	/// Parse tile params
    std::vector<Tile> tiles;
	if (!ExtractTiles(exportParams.tilex, exportParams.tiley, exportParams.tilez, tiles))
		return EXIT_FAILURE;

	const char* apiKey = exportParams.apiKey;
	LOG("Using API key %s\n", apiKey);

    std::unordered_map<Tile, std::unique_ptr<HeightData>> heightData;
    std::unordered_map<Tile, std::unique_ptr<TileData>> vectorTileData;

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
			auto tileData = DownloadTile(tile, apiKey);

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
        offset.x =  (tile.x - origin.x) * 2;
        offset.y = -(tile.y - origin.y) * 2;

        const auto& textureData = heightData[tile];

		/// Build terrain mesh
		if (exportParams.terrain)
		{
			if (!textureData)
				continue;

			/// Extract a plane geometry, vertices in [-1.0,1.0], for terrain mesh
			{
				//auto mesh = std::unique_ptr<PolygonMesh>(new PolygonMesh);
				auto mesh = new PolygonMesh(eLayerTerrain);

				buildPlane(mesh->vertices, mesh->indices, 2.0, 2.0, exportParams.terrainSubdivision, exportParams.terrainSubdivision);

				// Build terrain mesh extrusion, with bilinear height sampling
				for (auto& vertex : mesh->vertices)
				{
					v2 tilePosition = v2(vertex.position.x, vertex.position.y);
					float extrusion = sampleElevation(tilePosition, textureData);

					// Scale the height within the tile scale
					vertex.position.z = extrusion * tile.invScale;
				}

				/// Compute faces normals
				if (exportParams.normals)
					computeNormals(*mesh);

				mesh->offset = offset;
				//meshes.push_back(std::move(mesh));
				meshes.push_back(mesh);
			}

			/// Build pedestal
			if (exportParams.pedestal)
			{
				//auto ground = std::unique_ptr<PolygonMesh>(new PolygonMesh);
				//auto wall = std::unique_ptr<PolygonMesh>(new PolygonMesh);
				auto ground = new PolygonMesh(eLayerTerrain);
				auto wall = new PolygonMesh(eLayerTerrain);

				buildPlane(ground->vertices, ground->indices, 2.0, 2.0, exportParams.terrainSubdivision, exportParams.terrainSubdivision, true);

				for (auto& vertex : ground->vertices)
					vertex.position.z = exportParams.pedestalHeight * tile.invScale;

				buildPedestalPlanes(tile, wall->vertices, wall->indices, textureData, exportParams.terrainSubdivision, exportParams.pedestalHeight);

				ground->offset = offset;
				//meshes.push_back(std::move(ground));
				meshes.push_back(ground);
				wall->offset = offset;
				//meshes.push_back(std::move(wall));
				meshes.push_back(wall);
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
					/*const bool layerWater = layer.name == "water";
					const bool layerBuildings = layer.name == "buildings";
					const bool layerRoads = layer.name == "roads";*/

					//if (layerBuildings && !exportParams.buildings) continue;	// Skip buildings
					//if (layerRoads && !exportParams.roads) continue;			// Skip roads
					if (type == eLayerBuildings && !exportParams.buildings) continue;	// Skip buildings
					if (type == eLayerRoads && !exportParams.roads) continue;			// Skip roads
		
					/*std::vector<PolygonMesh*>*	meshArr = &meshes_other;
					if (layerBuildings)			meshArr = &meshes_buildings;
					else if (layerWater)		meshArr = &meshes_water;
					else if (layerRoads)		meshArr = &meshes_roads;*/

					const float default_height = type == eLayerBuildings ? exportParams.buildingsHeight * tile.invScale : 0.0f;

                    for (auto feature : layer.features) 
					{
                        //if (textureData && !layerBuildings && !layerRoads) continue;
						if (textureData && type != eLayerBuildings && type != eLayerRoads) continue;

						// Height
						float height = default_height;
						auto itHeight = feature.props.numericProps.find(keyHeight);
                        if (itHeight != feature.props.numericProps.end()) 
                            height = itHeight->second * scale;
         
                        //if (textureData && !layerRoads && height == 0.0f)
						if (textureData && type!=eLayerRoads && height == 0.0f)
                            continue;

						// Min height
						double minHeight = 0.0;
						auto itMinHeight = feature.props.numericProps.find(keyMinHeight);
                        if (itMinHeight != feature.props.numericProps.end())
                            minHeight = itMinHeight->second * scale;

						// JJ 
                        //auto mesh = std::unique_ptr<PolygonMesh>(new PolygonMesh);
						auto mesh = new PolygonMesh(type);

                        //if (exportParams.buildings)
						if(feature.geometryType == GeometryType::polygons)
						{
                            for (const Polygon2& polygon : feature.polygons)
							{
                                float centroidHeight = 0.f;
                                if (minHeight != height)
                                    centroidHeight = buildPolygonExtrusion(polygon, minHeight, height, mesh->vertices, mesh->indices, textureData, tile.invScale);

                                buildPolygon(polygon, height, mesh->vertices, mesh->indices, textureData, centroidHeight, tile.invScale);
                            }
                        }

                        if (exportParams.roads)
						{
							const float extrude = exportParams.roadsExtrusionWidth * tile.invScale;
                            for (Line& line : feature.lines)
							{
                                Polygon2 polygon;
                                polygon.emplace_back();
                                Line& polygonLine = polygon.back();

                                if (line.size() == 2)
								{
                                    v3 curr = line[0];
                                    v3 next = line[1];
                                    v3 n0 = perp(next - curr);

                                    polygonLine.push_back(curr - n0 * extrude);
                                    polygonLine.push_back(curr + n0 * extrude);
                                    polygonLine.push_back(next + n0 * extrude);
                                    polygonLine.push_back(next - n0 * extrude);
                                } 
								else 
								{
                                    v3 last = line[0];
                                    for (size_t i = 1; i < line.size() - 1; ++i)
									{
                                        v3 curr = line[i];
                                        v3 next = line[i+1];
                                        addPolygonPolylinePoint(polygonLine, curr, next, last, extrude, line.size(), i, true);
                                        last = curr;
                                    }

                                    last = line[line.size() - 1];
                                    for (int i = line.size() - 2; i > 0; --i)
									{
                                        v3 curr = line[i];
                                        v3 next = line[i-1];
                                        addPolygonPolylinePoint(polygonLine, curr, next, last, extrude, line.size(), i, false);
                                        last = curr;
                                    }
                                }

                                if (polygonLine.size() < 4) { continue; }

                                int count = 0;
                                for (size_t i = 0; i < polygonLine.size(); i++)
								{
                                    int j = (i + 1) % polygonLine.size();
                                    int k = (i + 2) % polygonLine.size();
                                    double z = (polygonLine[j].x - polygonLine[i].x)
                                             * (polygonLine[k].y - polygonLine[j].y)
                                             - (polygonLine[j].y - polygonLine[i].y)
                                             * (polygonLine[k].x - polygonLine[j].x);
                                    if (z < 0) { count--; }
                                    else if (z > 0) { count++; }
                                }

                                if (count > 0) { // CCW
                                    std::reverse(polygonLine.begin(), polygonLine.end());
                                }

                                // Close the polygon
                                polygonLine.push_back(polygonLine[0]);

                                size_t offset = mesh->vertices.size();

                                if (exportParams.roadsHeight > 0)
                                    buildPolygonExtrusion(polygon, 0.0, exportParams.roadsHeight * tile.invScale, mesh->vertices, mesh->indices, nullptr, tile.invScale);

                                buildPolygon(polygon, exportParams.roadsHeight * tile.invScale, mesh->vertices, mesh->indices, nullptr, 0.f, tile.invScale);

                                if (textureData) 
								{
                                    for (auto it = mesh->vertices.begin() + offset; it != mesh->vertices.end(); ++it) 
									{
                                        it->position.z += sampleElevation(v2(it->position.x, it->position.y), textureData) * tile.invScale;
                                    }
                                }
                            }

                            if (exportParams.normals && exportParams.terrain) {
                                computeNormals(*mesh);
                            }
                        }

                        // Add local mesh offset
                        mesh->offset = offset;
                        //meshes.push_back(std::move(mesh));
						meshes.push_back(mesh);
                    }
                }
            }
        }
    }

	LOG("Built PolygonMeshes in %.1fms\n", sw.GetMs());

	// Separate all meshes in it's respective layer
	std::vector<PolygonMesh*> meshArr[eNumLayerTypes];
	//for (PolygonMesh* mesh : meshes)
	//	meshArr[mesh->layerType].push_back(mesh);

	//PolygonMesh bigMesh[eNumLayerTypes];
	
	// Merge all meshes from same layer into 1 big mesh. If this mesh gets vcount>65536 the mesh is split
	for (PolygonMesh* mesh : meshes)
	{
		const int nv = mesh->vertices.size();
		
		// This mesh is to big. Throw it away
		if (nv > 65536)
		{
			gLogger.Warning("Mesh > 65536. Must implement splitting");
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

        bool success = aoBaked==1;
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
