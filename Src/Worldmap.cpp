#include "Precompiled.h"
#include "Worldmap.h"
#include "UI.h"
#include "../../../Source/Modules/Shared/SceneNew.h"
#include "../../../Source/Modules/Shared/State.h"
#include "../../../Source/Modules/Landscape/LandscapeSettings.h"
#include "State_Game.h"
#include "vectiler/jerry.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include "vectiler/projection.h"
#include "vectiler/geometry.h"

//-----------------------------------------------------------------------------
CStateManager	gStateManager;
CViewer			gViewer;
Entity*			root = NULL;
//-----------------------------------------------------------------------------
CViewer::CViewer()
{
	m_InputCamera = NULL;
	m_DrawCamera = NULL;
}
//-----------------------------------------------------------------------------
bool CViewer::Create()
{
	m_DefaultGameCam.PerspectiveY(50.0f, 0.5f, 10000.0f);
	//m_DefaultGameCam.PerspectiveY_ReversedDepth(50.0f, 0.5f);
	m_DebugCam.PerspectiveY(50.0f, 0.5f, 5000.0f);
	m_DefaultGameCam.SetName("Game Cam");
	m_DebugCam.SetName("Debug Cam");

	SetCamera(&m_DefaultGameCam);

	gScene.AddEntity(&m_DefaultGameCam);
	gScene.AddEntity(&m_DebugCam);


	SSunSettings* sun = gLandscapeSettings.Sun();
	sun->DiffuseModifier = 1.6f;
	sun->LightCol = Crgba(255,255,200);
	sun->AmbientModifier = 0.5f;
	sun->SkyCol = Crgba(100, 200, 255);
	sun->IndirectModifier = 0.3f;
	sun->IndirectCol = Crgba(255, 240, 210);
	
	sm_UpdateReduction = 0;
	sm_BiasLinear = 0.0006f;
	sm_ShadowDistance = 1000.0f;
	return true;
}
//-----------------------------------------------------------------------------
void CViewer::Destroy()
{
	gScene.RemoveEntity(&m_DebugCam);
	gScene.RemoveEntity(&m_DefaultGameCam);
}
//-----------------------------------------------------------------------------
int gMaterialCategory = -1;
//-----------------------------------------------------------------------------
bool MyApp_Init()
{
	Input_SetMouseMode(MOUSE_MODE_HIDDEN);
	Editor_Init();
	
	if (!gScene.Create(NULL))
		return false;

	gMaterialCategory = gResourceFactory.MatLib_Load("content_worldmap/materials.smt");

	CState_Game* state_game = new CState_Game("Game", "");
	state_game->SetFadeTimes(0.3f, 0.3f);
	gStateManager.AddState(state_game);
	gStateManager.SetActiveState(state_game);

	gViewer.Create();

	return true;
}
//-----------------------------------------------------------------------------
void MyApp_Deinit()
{
	gStateManager.DestroyStates();
	gScene.Destroy();
	gResourceFactory.MatLib_RemoveGroup(gMaterialCategory);
	Editor_Deinit();
}
//-----------------------------------------------------------------------------
void MyApp_InputEvent(const inputEvent_t* e)
{
	if (gStateManager.ProcessInput(e))
		return;
}
//-----------------------------------------------------------------------------
void MyApp_Update(float dt)
{
	CVec3 pos = gViewer.GetDrawCamera()->GetWorldPos();
	//DbgMsg("cam: <%.1f, %.1f, %.1f>", pos.x, pos.y, pos.z);
	gStateManager.Update(dt);
}
//-----------------------------------------------------------------------------
void MyApp_Render()
{
	gStateManager.Render();
	gStateManager.Render2d();
}
void MyApp_Resize() {}
//void MyApp_InputEvent(const inputEvent_t* e);
bool MyApp_Exit() 
{
	return true;
}