#pragma once
//-----------------------------------------------------------------------------
struct HeightData* DownloadHeightmapTile(const struct Tile& tile, const char* apiKey, float extrusionScale);
bool DownloadVectorTile(const struct Tile& tile, const char* apiKey, TArray<Layer>& layers);