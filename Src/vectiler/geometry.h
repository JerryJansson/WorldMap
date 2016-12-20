#pragma once
#include <vector>
#include "jerry.h"
//-----------------------------------------------------------------------------
struct PolygonVertex {
	v3 position;
	v3 normal;
};
//-----------------------------------------------------------------------------
enum ELayerType
{
	eLayerUnknown,		// 0

	eLayerTerrain,		// 1
	eLayerWater,		// 2
	eLayerBuildings,	// 3
	eLayerPlaces,		// 4
	eLayerTransit,		// 5
	eLayerPois,			// 6
	eLayerBoundaries,	// 7
	eLayerRoads,		// 8
	eLayerEarth,		// 9
	eLayerLanduse,		// 10

	eNumLayerTypes
};
//-----------------------------------------------------------------------------
extern const char* layerNames[eNumLayerTypes + 1];
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
void computeNormals(PolygonMesh* mesh);
//-----------------------------------------------------------------------------
bool saveOBJ(const char* outputOBJ,
	bool splitMeshes,
	std::vector<PolygonMesh*>& meshes,
	float offsetx,
	float offsety,
	bool append,
	bool normals);

bool AddMeshToMesh(const PolygonMesh* src, PolygonMesh* dst);

//bool SaveOBJ2(const char* fname, std::vector<PolygonMesh*> meshArr[eNumLayerTypes]);

bool SaveBin(const char* fname, std::vector<PolygonMesh*> meshArr[eNumLayerTypes]);
bool LoadBin(const char* fname, TArray<GGeom>& geomArr);

//-----------------------------------------------------------------------------
PolygonMesh* CreateMeshFromFeature(
	const ELayerType layerType,
	const struct Feature& feature,
	const float minHeight,
	const float height,
	const struct HeightData* heightMap,
	const float scale,				// tile.invScale
	const float lineExtrusionWidth, // params.roadsExtrusionWidth
	const float lineExtrusionHeight // params.roadsHeight
);
//-----------------------------------------------------------------------------
float sampleElevation(v2 position, const HeightData* heightMap);