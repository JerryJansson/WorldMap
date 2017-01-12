#pragma once
#include <vector>
#include "mapping.h"
//-----------------------------------------------------------------------------
struct StreamGeom
{
	ELayerType layerType;
	int layerSubIdx;
	TArray<vtxMap> vertices;
	TArray<uint16> indices;
};
//-----------------------------------------------------------------------------
bool SaveBin(const char* fname, const std::vector<struct PolygonMesh*>& meshes);
bool LoadBin(const char* fname, TArray<StreamGeom*>& geoms);