#pragma once
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
struct HeightData* DownloadHeightmapTile(const struct Tile& tile, const char* apiKey, float extrusionScale);
bool DownloadVectorTile(const struct Tile& tile, const char* apiKey, struct TileVectorData* data);