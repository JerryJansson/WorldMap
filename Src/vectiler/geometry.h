#pragma once
#include <vector>
#include <memory>
#include "jerry.h"
//-----------------------------------------------------------------------------
struct PolygonVertex {
	v3 position;
	v3 normal;
};
//-----------------------------------------------------------------------------
struct PolygonMesh
{
	std::vector<unsigned int> indices;
	std::vector<PolygonVertex> vertices;
	v2 offset;
};
//-----------------------------------------------------------------------------
void computeNormals(PolygonMesh& mesh);
//-----------------------------------------------------------------------------
bool saveOBJ(const char* outputOBJ,
	bool splitMeshes,
	std::vector<std::unique_ptr<PolygonMesh>>& meshes,
	float offsetx,
	float offsety,
	bool append,
	bool normals);