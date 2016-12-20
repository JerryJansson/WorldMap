#include "Precompiled.h"
#include "download.h"
#include <curl/curl.h>
//-----------------------------------------------------------------------------
MemoryStruct::MemoryStruct() { memory = NULL; size = 0; cap = 0; }
MemoryStruct::~MemoryStruct() { free(memory); }
void MemoryStruct::clear() { size = 0; }
void MemoryStruct::add(const char* data, int count)
{
	//LOG("Add: %db\n", count);
	int needed = size + count + 1;
	if (needed > cap)
	{
		cap = Max(cap * 2, needed);
		memory = (char*)realloc(memory, cap);
		//LOG("realloc: %db\n", cap);
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
bool DownloadData(MemoryStruct& out, const char* url)
{
	static bool curlInitialized = false;
	static CURL* curl = nullptr;

	if (!curlInitialized)
	{
		curl_global_init(CURL_GLOBAL_DEFAULT);
		curl = curl_easy_init();
		curlInitialized = true;

		// set up curl to perform fetch
		//curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, writeData);
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
		LOG("Downloaded tile in %.1fms\n", sw.GetMs());
	else
		LOG(" -- Failure: %s\n", curl_easy_strerror(result));

	return result == CURLE_OK;// && out.rdbuf()->in_avail();
}
//-----------------------------------------------------------------------------
/*inline std::string vectorTileURL(const Tile& tile, const std::string& apiKey) {
	return "https://tile.mapzen.com/mapzen/vector/v1/all/"
		+ std::to_string(tile.z) + "/"
		+ std::to_string(tile.x) + "/"
		+ std::to_string(tile.y) + ".json?api_key=" + apiKey;
}
//-----------------------------------------------------------------------------
inline std::string terrainURL(const Tile& tile, const std::string& apiKey) {
	return "https://tile.mapzen.com/mapzen/terrain/v1/terrarium/"
		+ std::to_string(tile.z) + "/"
		+ std::to_string(tile.x) + "/"
		+ std::to_string(tile.y) + ".png?api_key=" + apiKey;
}*/
//-----------------------------------------------------------------------------
CStrL VectorTileURL(int x, int y, int z, const char* apiKey)
{
	return Str_Printf("https://tile.mapzen.com/mapzen/vector/v1/all/%d/%d/%d.json?api_key=%s", z, x, y, apiKey);
}
//-----------------------------------------------------------------------------
CStrL TerrainURL(int x, int y, int z, const char* apiKey)
{
	return Str_Printf("https://tile.mapzen.com/mapzen/terrain/v1/terrarium/%d/%d/%d.png?api_key=%s", z,x,y, apiKey);
}