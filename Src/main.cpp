#include "Precompiled.h"
#include "../../../Source/Modules/Terrain/TerrainMisc.h"
#include <shellapi.h>
int test(int x, int y, int z)
{
	return x^y^z;
}
//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int w, h;
	E_GetMonitorResolution(w, h);
	w *= 0.95f;
	h *= 0.95f;
	
	if (!Engine_Init(NULL, "World Map", "1.0", w, h, "", "sandbox_worldmap/"))//, eInvisible/*|eFullScreen*/))
		return 0;

	CVec2 a(1,1), b(2,2);
	CVec2 c = a + b * 0.5f;

	//E_SetWindowPos(550, 550);
	//E_ShowWindow(true);
	
	/*for (int i = 0; i < 20; i++)
	{
		int h = test(0, i, 16);
		LOG("%d, %d, %d - %d\n", 0, i, 16, h);

		h = test(i, i, 16);
		LOG("%d, %d, %d - %d\n", i, i, 16, h);

		h = test(12, i, 16);
		LOG("%d, %d, %d - %d\n", 12, i, 16, h);
	}
	int a = test(1, 1, 16);
	int b = test(1, 2, 16);
	int c = test(16984, 55911, 16);
	*/

	gConsole.ExecuteFile("content_simview2/engine.cfg");

	while (Engine_DoFrame()) {}

	Engine_Deinit();

	return 0;
}