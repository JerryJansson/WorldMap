#pragma once
#include "projection.h"
#include <vector>
//-----------------------------------------------------------------------------
struct MemoryStruct
{
	char *memory;
	int size;
	int cap;

	MemoryStruct();
	~MemoryStruct();
	void clear();
	void add(const char* data, int count);
};
//-----------------------------------------------------------------------------
struct HeightData {
	std::vector<std::vector<float>> elevation;
	int width;
	int height;
};
//-----------------------------------------------------------------------------
//CStrL VectorTileURL(int x, int y, int z, const char* apiKey);
//CStrL TerrainURL(int x, int y, int z, const char* apiKey);
//bool DownloadData(MemoryStruct& out, const char* url);
HeightData* DownloadHeightmapTile(const Tile& tile, const char* apiKey, float extrusionScale);
struct TileVectorData* DownloadVectorTile(const Tile& tile, const char* apiKey);