#include "Precompiled.h"
#include "../../../Source/Modules/Terrain/TerrainMisc.h"
#include <shellapi.h>

//-----------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int w, h;
	E_GetMonitorResolution(w, h);
	w *= 0.95f;
	h *= 0.95f;
	
	if (!Engine_Init(NULL, "SimView2", "1.0", w, h, "", "sandbox_worldmap/"))//, eInvisible/*|eFullScreen*/))
		return 0;

	//E_SetWindowPos(550, 550);
	//E_ShowWindow(true);


	gConsole.ExecuteFile("content_simview2/engine.cfg");

	while (Engine_DoFrame()) {}

	Engine_Deinit();

	return 0;
}