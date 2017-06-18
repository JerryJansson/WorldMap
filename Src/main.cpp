#include "Precompiled.h"
#include "../../../Source/Modules/Terrain/TerrainMisc.h"

#ifndef JJ_WORLDMAP
	error
#endif
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int w, h;
	E_GetMonitorResolution(w, h);
	w *= 0.95f;
	h *= 0.95f;
	
	if (!Engine_Init(NULL, "World Map", "1.0", w, h, "", "sandbox_worldmap/"))
		return 0;

	/*int a = 0;
	std::vector<int> v;
	int c = v.capacity();
	LOG("s: %d, c: %d\n", v.size(), v.capacity());
	for (int i = 0; i < 1000; i++)
	{
		v.push_back(i);
		if (v.capacity() != c)
		{
			a++;
			c = v.capacity();
		}
		LOG("s: %d, c: %d\n", v.size(), v.capacity());
	}
	LOG("Allocs: %d\n", a);*/

	gConsole.ExecuteFile("content_simview2/engine.cfg");

	while (Engine_DoFrame()) {}

	Engine_Deinit();

	return 0;
}