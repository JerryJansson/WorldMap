#pragma once
//#include <vector>
//#include "mapping.h"
#include "tiledata.h"
#include "earcut.h"
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
class PolyMeshBuilder
{
public:
	mapbox::detail::Earcut<uint32_t> earcut;
	TArray<Vec2i> flatten;
	TArray<int> remap;

public:
	void Clear()
	{
		flatten.Clear();
		remap.Clear();
		//flatten.EnsureCapacity(256);
	}
};
//-----------------------------------------------------------------------------
void computeNormals(PolygonMesh* mesh);
//-----------------------------------------------------------------------------
PolygonMesh* CreatePolygonMeshFromFeature(const ELayerType layerType, const struct Feature* feature, const struct HeightData* heightMap, PolyMeshBuilder& ctx);
//-----------------------------------------------------------------------------
float sampleElevation(v2 position, const HeightData* heightMap);