#pragma once
#include <vector>
#include "jerry.h"
#include "mapping.h"
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
	ELayerType layerType;
	const struct Feature* feature;

	PolygonMesh(ELayerType type, const struct Feature* _feature=NULL) : layerType(type), feature(_feature) {}
};
//-----------------------------------------------------------------------------
void computeNormals(PolygonMesh* mesh);
//-----------------------------------------------------------------------------
PolygonMesh* CreateMeshFromFeature(const ELayerType layerType, const struct Feature* feature, const struct HeightData* heightMap);
//-----------------------------------------------------------------------------
float sampleElevation(v2 position, const HeightData* heightMap);