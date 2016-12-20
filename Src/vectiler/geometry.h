#pragma once
#include <vector>
//#include <memory>
#include "jerry.h"
//-----------------------------------------------------------------------------
struct PolygonVertex {
	v3 position;
	v3 normal;
};
//-----------------------------------------------------------------------------
enum ELayerType
{
	eLayerTerrain,
	eLayerBuildings,
	eLayerRoads,
	eLayerWater,
	eLayerOther,

	eNumLayerTypes
};
//-----------------------------------------------------------------------------
extern const char* layerNames[eNumLayerTypes];
//-----------------------------------------------------------------------------
struct PolygonMesh
{
	std::vector<unsigned int> indices;
	std::vector<PolygonVertex> vertices;
	v2 offset;
	ELayerType layerType;

	PolygonMesh(ELayerType type) : layerType(type) {}
};
//-----------------------------------------------------------------------------
void computeNormals(PolygonMesh& mesh);
//-----------------------------------------------------------------------------
bool saveOBJ(const char* outputOBJ,
	bool splitMeshes,
	//std::vector<std::unique_ptr<PolygonMesh>>& meshes,
	std::vector<PolygonMesh*>& meshes,
	float offsetx,
	float offsety,
	bool append,
	bool normals);

bool AddMeshToMesh(const PolygonMesh* src, PolygonMesh* dst);

bool SaveOBJ2(const char* fname, std::vector<PolygonMesh*> meshArr[eNumLayerTypes]);

bool SaveBin(const char* fname, std::vector<PolygonMesh*> meshArr[eNumLayerTypes]);
bool LoadBin(const char* fname);