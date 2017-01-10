#include "Precompiled.h"
#include "download.h"
#include "tiledata.h"
#include "geojson.h"
#include "rapidjson/document.h"
#include <curl/curl.h>
//#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//-----------------------------------------------------------------------------
MemoryStruct::MemoryStruct() { memory = NULL; size = 0; cap = 0; }
MemoryStruct::~MemoryStruct() { free(memory); }
void MemoryStruct::clear() { size = 0; }
void MemoryStruct::add(const char* data, int count)
{
	int needed = size + count + 1;
	if (needed > cap)
	{
		cap = Max(cap * 2, needed);
		memory = (char*)realloc(memory, cap);
	}
	memcpy(&(memory[size]), data, count);
	size += count;
	memory[size] = 0;
}
//-----------------------------------------------------------------------------
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	MemoryStruct *mem = (MemoryStruct *)userp;
	mem->add((char*)contents, realsize);
	return realsize;
}
//-----------------------------------------------------------------------------
// NOT THREAD SAFE!
// Probably need separate CURL* curl handles for each thread
//-----------------------------------------------------------------------------
bool DownloadData(MemoryStruct& out, const char* url)
{
	static CURL* curl = NULL;

	if (!curl)
	{
		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();

		// set up curl to perform fetch
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
		curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
		//curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip");
		curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, -1);
	}

	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &out);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	LOG("URL request: %s\n", url);

	CStopWatch sw;
	CURLcode result = curl_easy_perform(curl);

	if (result == CURLE_OK)
		LOG("Downloaded tile in %.1fms (%d kb uncompressed)\n", sw.GetMs(), out.size/1024);
	else
		LOG(" -- Failure: %s\n", curl_easy_strerror(result));

	return result == CURLE_OK;// && out.rdbuf()->in_avail();
}
//-----------------------------------------------------------------------------
CStrL VectorTileURL(int x, int y, int z, const char* apiKey)
{
	Vec2i google = TmsToGoogleTile(Vec2i(x, y), z);
	return Str_Printf("https://tile.mapzen.com/mapzen/vector/v1/all/%d/%d/%d.json?api_key=%s", z, google.x, google.y, apiKey);
}
//-----------------------------------------------------------------------------
CStrL TerrainURL(int x, int y, int z, const char* apiKey)
{
	Vec2i google = TmsToGoogleTile(Vec2i(x, y), z);
	return Str_Printf("https://tile.mapzen.com/mapzen/terrain/v1/terrarium/%d/%d/%d.png?api_key=%s", z, google.x, google.y, apiKey);
}
//-----------------------------------------------------------------------------
HeightData* DownloadHeightmapTile(const Tile& tile, const char* apiKey, float extrusionScale)
{
	CStrL url = TerrainURL(tile.x, tile.y, tile.z, apiKey);
	MemoryStruct out;
	if (!DownloadData(out, url))
		return nullptr;

	// Decode texture PNG
	int width, height, comp;
	unsigned char* pixels = stbi_load_from_memory((stbi_uc*)out.memory, out.size, &width, &height, &comp, STBI_rgb_alpha);

	if (comp != STBI_rgb_alpha)
	{
		printf("Failed to decompress PNG file\n");
		return nullptr;
	}

	HeightData* data = new HeightData();

	data->elevation.resize(height);
	for (int i = 0; i < height; ++i)
		data->elevation[i].resize(width);

	data->width = width;
	data->height = height;

	unsigned char* pixel = pixels;
	for (int i = 0; i < width * height; i++, pixel += 4)
	{
		float red = *(pixel + 0);
		float green = *(pixel + 1);
		float blue = *(pixel + 2);

		// Decode the elevation packed data from color component
		float elevation = (red * 256 + green + blue / 256) - 32768;

		int y = i / height;
		int x = i % width;

		assert(x >= 0 && x <= width && y >= 0 && y <= height);

		data->elevation[x][y] = elevation * extrusionScale;
	}

	return data;
}
//-----------------------------------------------------------------------------
bool DownloadVectorTile(const Tile& tile, const char* apiKey, TileVectorData* data)
{
	CStrL url = VectorTileURL(tile.x, tile.y, tile.z, apiKey);
	MemoryStruct out;
	if (!DownloadData(out, url))
		return false;

	// Parse written data into a JSON object
	CStopWatch sw;
	rapidjson::Document doc;
	doc.Parse(out.memory);

	if (doc.HasParseError())
	{
		LOG("Error parsing tile\n");
		rapidjson::ParseErrorCode code = doc.GetParseError();
		size_t errOffset = doc.GetErrorOffset();
		return false;
	}

	float tJson = sw.GetMs(true);

	//TileVectorData* data = new TileVectorData();
	for (auto layer = doc.MemberBegin(); layer != doc.MemberEnd(); ++layer)
	{
		data->layers.emplace_back(layer->name.GetString());
		GeoJson::extractLayer(layer->value, data->layers.back(), tile);
	}

	float tLayers = sw.GetMs();

	LOG("Parsed json in %.1fms. Built GeoJson structures in %.1fms\n", tJson, tLayers);

	//return data;
	return true;
}