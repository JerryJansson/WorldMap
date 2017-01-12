#include "Precompiled.h"
#include "geometry.h"
#include "tiledata.h"
#include "earcut.h"
//-----------------------------------------------------------------------------
#define EPSILON 1e-5f
//-----------------------------------------------------------------------------
v3 perp(const v3& v) { return glm::normalize(v3(-v.y, v.x, 0.0)); }
//-----------------------------------------------------------------------------
void computeNormals(PolygonMesh* mesh)
{
	for (size_t i = 0; i < mesh->indices.size(); i += 3)
	{
		int i1 = mesh->indices[i + 0];
		int i2 = mesh->indices[i + 1];
		int i3 = mesh->indices[i + 2];

		const v3& p1 = mesh->vertices[i1].position;
		const v3& p2 = mesh->vertices[i2].position;
		const v3& p3 = mesh->vertices[i3].position;

		v3 d = myNormalize(myCross(p2 - p1, p3 - p1));

		mesh->vertices[i1].normal += d;
		mesh->vertices[i2].normal += d;
		mesh->vertices[i3].normal += d;
	}

	for (auto& v : mesh->vertices) {
		v.normal = myNormalize(v.normal);
	}
}
/*-----------------------------------------------------------------------------
* Sample elevation using bilinear texture sampling
* - position: must lie within tile range [-1.0, 1.0]
* - textureData: the elevation tile data, may be null
//-----------------------------------------------------------------------------*/
float sampleElevation(v2 position, const HeightData* heightMap)
{
	if (!heightMap)
		return 0.0f;
	
	position.x = Clamp(position.x, -1.0f, 1.0f);
	position.y = Clamp(position.y, -1.0f, 1.0f);

	// Normalize vertex coordinates into the texture coordinates range
	float u = (position.x * 0.5f + 0.5f) * heightMap->width;
	float v = (position.y * 0.5f + 0.5f) * heightMap->height;

	// Flip v coordinate according to tile coordinates
	v = heightMap->height - v;

	float alpha = u - floor(u);
	float beta = v - floor(v);

	int ii0 = floor(u);
	int jj0 = floor(v);
	int ii1 = ii0 + 1;
	int jj1 = jj0 + 1;

	// Clamp on borders
	ii0 = std::min(ii0, heightMap->width - 1);
	jj0 = std::min(jj0, heightMap->height - 1);
	ii1 = std::min(ii1, heightMap->width - 1);
	jj1 = std::min(jj1, heightMap->height - 1);

	// Sample four corners of the current texel
	float s0 = heightMap->elevation[ii0][jj0];
	float s1 = heightMap->elevation[ii0][jj1];
	float s2 = heightMap->elevation[ii1][jj0];
	float s3 = heightMap->elevation[ii1][jj1];

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
	v2 centroid(0, 0);
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
float buildPolygonExtrusion(const Polygon2& polygon, const float minHeight,	const float height,
	std::vector<PolygonVertex>& outVertices,
	std::vector<unsigned int>& outIndices,
	const HeightData* elevation)
{
	int vertexDataOffset = outVertices.size();
	float minz = 0.f;
	float cz = 0.f;

	// Compute min and max height of the polygon
	if (elevation)
	{
		// The polygon centroid height
		cz = sampleElevation(centroid(polygon), elevation);
		minz = std::numeric_limits<float>::max();

		for (auto& linestring : polygon)
		{
			for (size_t i = 0; i < linestring.size(); i++)
			{
				v3 p(linestring[i]);
				float pz = sampleElevation(v2(p.x, p.y), elevation);
				minz = std::min(minz, pz);
			}
		}
	}

	const v3 upVector(0.0f, 0.0f, 1.0f);
	for (auto& linestring : polygon)
	{
		const size_t lineSize = linestring.size();

		outVertices.reserve(outVertices.size() + lineSize * 4);
		outIndices.reserve(outIndices.size() + lineSize * 6);

		for (size_t i = 0; i < lineSize - 1; i++)
		{
			v3 a(linestring[i]);
			v3 b(linestring[i + 1]);

			if (a == b) { continue; }

			v3 normalVector = glm::cross(upVector, b - a);
			normalVector = glm::normalize(normalVector);

			a.z = height + cz;
			outVertices.push_back({ a, normalVector });
			b.z = height + cz;
			outVertices.push_back({ b, normalVector });
			a.z = minHeight + minz;
			outVertices.push_back({ a, normalVector });
			b.z = minHeight + minz;
			outVertices.push_back({ b, normalVector });

			outIndices.push_back(vertexDataOffset + 0);
			outIndices.push_back(vertexDataOffset + 1);
			outIndices.push_back(vertexDataOffset + 2);
			outIndices.push_back(vertexDataOffset + 1);
			outIndices.push_back(vertexDataOffset + 3);
			outIndices.push_back(vertexDataOffset + 2);

			vertexDataOffset += 4;
		}
	}

	return cz;
}
//-----------------------------------------------------------------------------
void buildPolygon(const Polygon2& polygon, const float height,
	std::vector<PolygonVertex>& outVertices,
	std::vector<unsigned int>& outIndices,
	const HeightData* elevation,
	float centroidHeight)
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

	for (auto& p : earcut.vertices)
	{
		v2 position(p[0], p[1]);
		v3 coord(position.x, position.y, height + centroidHeight);
		outVertices.push_back({ coord, normal });
	}
}
//-----------------------------------------------------------------------------
v3 computeMiterVector(const v3& d0, const v3& d1, const v3& n0, const v3& n1)
{
	v3 miter = glm::normalize(n0 + n1);
	float miterl2 = glm::dot(miter, miter);

	if (miterl2 < std::numeric_limits<float>::epsilon()) {
		miter = v3(n1.y - n0.y, n0.x - n1.x, 0.0);
	}
	else {
		float theta = atan2f(d1.y, d1.x) - atan2f(d0.y, d0.x);
		if (theta < 0.f) 
			theta += 2.0f * FLOAT_PI;
		miter *= 1.f / std::max<float>(sin(theta * 0.5f), EPSILON);
	}

	return miter;
}
//-----------------------------------------------------------------------------
void addPolygonPolylinePoint(LineString& linestring,
	const v3& curr,
	const v3& next,
	const v3& last,
	const float extrude,
	const size_t lineDataSize,
	const size_t i,
	const bool forward)
{
	const v3 n0 = perp(curr - last);
	const v3 n1 = perp(next - curr);
	bool right = glm::cross(n1, n0).z > 0.0;

	if ((i == 1 && forward) || (i == lineDataSize - 2 && !forward))
	{
		linestring.push_back(last + n0 * extrude);
		linestring.push_back(last - n0 * extrude);
	}

	if (right)
	{
		v3 d0 = glm::normalize(last - curr);
		v3 d1 = glm::normalize(next - curr);
		v3 miter = computeMiterVector(d0, d1, n0, n1);
		linestring.push_back(curr - miter * extrude);
	}
	else
	{
		linestring.push_back(curr - n0 * extrude);
		linestring.push_back(curr - n1 * extrude);
	}
}
//-----------------------------------------------------------------------------
// 1150
// 115
// 31508
//
// 1134
// 107
// 31981
//
// 1163
// 108
// 31362
//
// Less CCW stuff
// 1122
// 105
// 30918
// 
//-----------------------------------------------------------------------------
PolygonMesh* CreateMeshFromFeature(const ELayerType layerType, const Feature* f, const HeightData* heightMap)
{
	CStopWatch sw;

	if (strstr(f->name.Str(), "Deutsches Patent") != 0)
	{
		int abba = 10;
	}

	if (f->lineStrings.size() > 3000)
	{
		int abba = 10;
	}

	if (f->geometryType == GeometryType::unknown || f->geometryType == GeometryType::points)
		return NULL;

	auto mesh = new PolygonMesh(layerType, f);
	const float sortHeight = f->sort_rank / (500.0f);

	if (f->geometryType == GeometryType::polygons)
	{
		for (const Polygon2& polygon : f->polygons)
		{
			if (f->min_height > f->height)
			{
				int abba = 10;
			}

			float centroidHeight = 0.f;
			if (f->min_height != f->height)
			{
				centroidHeight = buildPolygonExtrusion(polygon, f->min_height, f->height, mesh->vertices, mesh->indices, heightMap);
				buildPolygon(polygon, f->height, mesh->vertices, mesh->indices, heightMap, centroidHeight);
			}
			else
				buildPolygon(polygon, f->height + sortHeight, mesh->vertices, mesh->indices, heightMap, centroidHeight);
		}
	}
	else if (f->geometryType == GeometryType::lines)
	{
		float t1 = 0;
		float t2 = 0;
		CStopWatch sw1;
		const float extrudeW = f->road_width;// *scale;
		//const float extrudeH = lineExtrusionHeight * scale;
		const float extrudeH = sortHeight;

		Polygon2 polygon;
		polygon.emplace_back();
		LineString& polygonLine = polygon.back();

		for (const LineString& linestring : f->lineStrings)
		{
			polygonLine.clear();
			const size_t n = linestring.size();

			sw1.Start();
			if (n == 2)
			{
				const v3& curr = linestring[0];
				const v3& next = linestring[1];
				v3 n0 = perp(next - curr);
				n0 *= extrudeW;

				polygonLine.push_back(curr - n0);
				polygonLine.push_back(curr + n0);
				polygonLine.push_back(next + n0);
				polygonLine.push_back(next - n0);
			}
			else
			{
				v3 last = linestring[0];
				for (size_t i = 1; i < n - 1; ++i)
				{
					const v3& curr = linestring[i];
					const v3& next = linestring[i + 1];
					addPolygonPolylinePoint(polygonLine, curr, next, last, extrudeW, n, i, true);
					last = curr;
				}

				last = linestring[n - 1];
				for (int i = n - 2; i > 0; --i)
				{
					const v3& curr = linestring[i];
					const v3& next = linestring[i - 1];
					addPolygonPolylinePoint(polygonLine, curr, next, last, extrudeW, n, i, false);
					last = curr;
				}

				std::reverse(polygonLine.begin(), polygonLine.end());
			}

			if (polygonLine.size() < 4) { continue; }

			/*int count = 0;
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

			if (line.size() == 2 && count > 0)
			{
				int abba = 10;
			}
			if (line.size() > 2 && count <= 0)
			{
				int abba = 10;
			}

			if (count > 0) { // CCW

				if (line.size() == 2)
				{
					int abba = 10;
				}
				
				std::reverse(polygonLine.begin(), polygonLine.end());
			}*/

			// Close the polygon
			polygonLine.push_back(polygonLine[0]);

			const size_t offset = mesh->vertices.size();
			t1 += sw1.GetMs(true);

			if (extrudeH > 0)
				buildPolygonExtrusion(polygon, 0.0f, extrudeH, mesh->vertices, mesh->indices, nullptr);

			buildPolygon(polygon, extrudeH, mesh->vertices, mesh->indices, nullptr, 0.f);

			t2 += sw1.GetMs();

			if (heightMap)
			{
				for (auto it = mesh->vertices.begin() + offset; it != mesh->vertices.end(); ++it)
				{
					it->position.z += sampleElevation(v2(it->position.x, it->position.y), heightMap);
				}
			}
		}

		//if (params.terrain)
		if (heightMap)
			computeNormals(mesh);

		float time = sw.GetMs();
		if (time > 50)
		{
			LOG("Built mesh from featId(%I64d). Time %.1fms. V: %d, T: %d\n", f->id, time, mesh->vertices.size(), mesh->indices.size() / 3);
			LOG("LineBuilder: %.1f. PolyBuilder: %.1f\n", t1, t2);
			//LOG("FeatureType: %d\n", feature.geometryType);
		}
	}

	if (mesh->vertices.size() == 0)
	{
		delete mesh;
		mesh = NULL;
	}

	return mesh;
}
//-----------------------------------------------------------------------------
#if 0
PolygonMesh* CreateMeshFromFeature(const ELayerType layerType, const Feature& feature, const HeightData* heightMap)
{
	CStopWatch sw;

	if (feature.lines.size() > 3000)
	{
		int abba = 10;
	}

	if (feature.geometryType == GeometryType::unknown || feature.geometryType == GeometryType::points)
		return NULL;

	auto mesh = new PolygonMesh(layerType);
	const float sortHeight = feature.sort_rank / (500.0f);

	//if (exportParams.buildings)
	if (feature.geometryType == GeometryType::polygons)
	{
		for (const Polygon2& polygon : feature.polygons)
		{
			if (feature.min_height > feature.height)
			{
				int abba = 10;
			}

			float centroidHeight = 0.f;
			if (feature.min_height != feature.height)
			{
				centroidHeight = buildPolygonExtrusion(polygon, feature.min_height, feature.height, mesh->vertices, mesh->indices, heightMap);
				buildPolygon(polygon, feature.height, mesh->vertices, mesh->indices, heightMap, centroidHeight);
			}
			else
				buildPolygon(polygon, feature.height + sortHeight, mesh->vertices, mesh->indices, heightMap, centroidHeight);
		}
	}
	else if (feature.geometryType == GeometryType::lines)
	{
		if (layerType == eLayerRoads)
		{
			int abba = 10;
		}
		else
		{
			int abba = 10;
		}
		const float extrudeW = feature.road_width;// *scale;
		//const float extrudeH = lineExtrusionHeight * scale;
		const float extrudeH = sortHeight;

		for (const Line& line : feature.lines)
		{
			Polygon2 polygon;
			polygon.emplace_back();
			Line& polygonLine = polygon.back();

			if (line.size() == 2)
			{
				v3 curr = line[0];
				v3 next = line[1];
				v3 n0 = perp(next - curr);

				polygonLine.push_back(curr - n0 * extrudeW);
				polygonLine.push_back(curr + n0 * extrudeW);
				polygonLine.push_back(next + n0 * extrudeW);
				polygonLine.push_back(next - n0 * extrudeW);
			}
			else
			{
				v3 last = line[0];
				for (size_t i = 1; i < line.size() - 1; ++i)
				{
					v3 curr = line[i];
					v3 next = line[i + 1];
					addPolygonPolylinePoint(polygonLine, curr, next, last, extrudeW, line.size(), i, true);
					last = curr;
				}

				last = line[line.size() - 1];
				for (int i = line.size() - 2; i > 0; --i)
				{
					v3 curr = line[i];
					v3 next = line[i - 1];
					addPolygonPolylinePoint(polygonLine, curr, next, last, extrudeW, line.size(), i, false);
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

			if (extrudeH > 0)
				buildPolygonExtrusion(polygon, 0.0f, extrudeH, mesh->vertices, mesh->indices, nullptr);// , scale);

			buildPolygon(polygon, extrudeH, mesh->vertices, mesh->indices, nullptr, 0.f);// , scale);

			if (heightMap)
			{
				for (auto it = mesh->vertices.begin() + offset; it != mesh->vertices.end(); ++it)
				{
					it->position.z += sampleElevation(v2(it->position.x, it->position.y), heightMap);// *scale;
				}
			}
		}

		//if (params.terrain)
		if(heightMap)
			computeNormals(mesh);
	}

	float time = sw.GetMs();
	if (time > 100)
	{
		LOG("Built mesh from featId(%I64d). Time %.1fms. V: %d, T: %d\n", feature.id, time, mesh->vertices.size(), mesh->indices.size() / 3);
	}
	
	if (mesh->vertices.size() == 0)
	{
		delete mesh;
		mesh = NULL;
	}

	return mesh;
}
#endif