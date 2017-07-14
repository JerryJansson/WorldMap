#pragma once
#include "tiledata.h"
#include "earcut.h"
//-----------------------------------------------------------------------------
struct PolyVert {
	v3 position;
	v3 normal;
};
//typedef std::vector<unsigned int>	IdxArr;
//typedef std::vector<PolyVert>		VtxArr;
typedef TArray<uint32> IdxArr;
typedef TArray<PolyVert> VtxArr;
//-----------------------------------------------------------------------------
struct PolygonMesh
{
	IdxArr indices;
	VtxArr vertices;
	ELayerType layerType;
	//const struct Feature* feature;
	CStr			feature_name;
	int				feature_sortrank;
	EFeatureKind	feature_kind;

	PolygonMesh(ELayerType type, const struct Feature* _feature)// : layerType(type), feature(_feature) {}
	{
		layerType = type;
		if (_feature)
		{
			feature_name = _feature->name;
			feature_sortrank = _feature->sort_rank;
			feature_kind = _feature->kind;
		}
		else
		{
			feature_name = "Unknown";
			feature_sortrank = 0;
			feature_kind = eKind_unknown;
		}
	}
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