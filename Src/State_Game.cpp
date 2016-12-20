#include "Precompiled.h"
#include "UI.h"
#include "../../../Source/Modules/Shared/SceneNew.h"
#include "../../../Source/Modules/Shared/State.h"
//#include "../../../Source/Modules/Terrain/Terrain.h"
//#include "../../../Source/Modules/GameObject/GameObject.h"
#include "../../../Source/Modules/Landscape/LandscapeEnvironment.h"
//#include "../../../Source/Modules/Landscape/LandscapeSettings.h"
#include "State_Game.h"
//#include "SimView.h"
#include "Worldmap.h"


bool allowGuiInput = true;
//-----------------------------------------------------------------------------
void EnableEditor(bool on)
{
#ifdef _USE_EDITOR_
	gEditorEnabled = on;
#else
	on = false;
#endif
	Input_SetMouseMode(on ? MOUSE_MODE_NORMAL : MOUSE_MODE_HIDDEN_LOCKED);

	if (on)
	{
		allowGuiInput = true;
		gViewer.SetInputCamera(NULL);
	}
	else
	{
		gViewer.SetCamera(gViewer.GetDefaultCamera());
	}
}
//-----------------------------------------------------------------------------
CVar c_LockCamToGround("c_LockCamToGround", false);
//-----------------------------------------------------------------------------
bool CState_Game::Init()
{
	if (!CState::Init())
		return false;

	EnableEditor(false);
	return true;
}
//-----------------------------------------------------------------------------
void CState_Game::Deinit()
{
	UnloadCourse(gViewer.GetGameCamera());
	gViewer.Destroy();
	CState::Deinit();
}
//-----------------------------------------------------------------------------
TArray<vtxPN> tracep;
CVec3 gDebugTrace;

bool inputWasHandledbyImGui = false;
//-----------------------------------------------------------------------------
bool CState_Game::ProcessInput(const inputEvent_t* e)
{
	if (inputWasHandledbyImGui)
		return true;

	if (e->type == KB_DOWN)
	{
		// Toggle game/editor - mode
#ifdef _USE_EDITOR_
		if (e->idx == KEY_F1)
		{
			EnableEditor(!gEditorEnabled);
			return true;
		}
#endif

		//if (e->idx == KEY_ESC)	{ m_pManager->SetActiveState("Menu"); return true; }

		if (!gEditorEnabled)
		{
			if (e->idx == ' ')		{ gViewer.ToggleDebugCamera(false); return true; }
			if (e->idx == 'I')		{ gViewer.ToggleDebugCamera(true);	 return true; }
		}
	}

	if (gEditorEnabled)
	{
		if (e->type == MOUSE_RDOWN)
		{
			Input_SetMouseMode(MOUSE_MODE_HIDDEN_LOCKED);
			allowGuiInput = false;
			gViewer.SetInputCamera(gViewer.GetGameCamera());
			return true;
		}
		if (e->type == MOUSE_RUP)
		{
			Input_SetMouseMode(MOUSE_MODE_NORMAL);
			allowGuiInput = true;
			gViewer.SetInputCamera(NULL);
			return true;
		}
	}

	// Editor
//	if (Editor_ProcessInput(e))
	//	return true;

	return false;
}

//-----------------------------------------------------------------------------
void CState_Game::Update(const float dt)
{
	CState::Update(dt);
	
	if (gEditorEnabled)
		inputWasHandledbyImGui = Editor_Update(dt, allowGuiInput);

	if (!inputWasHandledbyImGui)
	{
		/*if (MOUSEBUTTON(M_LEFT))
		{
			DebugTrace();
			float h = gTerrain.GetLandscapeHeight(gDebugTrace);
			TerrainSurface* s = gTerrain.GetSurfaceinfo(gDebugTrace);
			if (s)
			{
				DbgMsg("Trace: %s", s->name.Str());
				DbgMsg("Height: <%.3f>", h);
			}
		}
		if (MOUSEBUTTONONCE(M_RIGHT))
		{
			CVec3 pos = gViewer.GetGameCamera()->GetWorldPos() + CVec3(0, 0, -1025);
			float h = gTerrain.GetLandscapeHeight_EntireWorldOnDisc(pos, NULL);
			DbgMsgTimed(10.0f, "Queried: <%.1f, %.1f, %.1f> = %.2f", pos.x, pos.y, pos.z, h);
		}*/
	}

	/*if (c_LockCamToGround)
	{
		CCamera* cam = gViewer.GetGameCamera();
		CVec3 p = cam->GetWorldPos();
		float y = gTerrain.GetLandscapeHeight(p) + 1.8f;
		p.y = y;
		cam->SetWorldPos(p);
	}*/

	gScene.Update(gViewer.GetGameCamera(), dt);

	//if (!gEditorEnabled)
	//	gViewer.m_pMode->Update(dt);
}
//-----------------------------------------------------------------------------
void CState_Game::Render()
{
	for (int i = 0; i < tracep.Num(); i++)
	{
		DrawWireCube(tracep[i].pos, 0.02f, gRGBA_Red);
		DrawLine3d(tracep[i].pos, tracep[i].pos + tracep[i].nrm, gRGBA_Red);
	}

	CCamera* gameCam = gViewer.GetGameCamera();
	CCamera* viewCam = gViewer.GetDrawCamera();

	gScene.Render(gameCam, viewCam, NULL);

	//if (gEditorEnabled)
	//	Editor_Render3d(viewCam);
}
//-----------------------------------------------------------------------------
void CState_Game::Render2d()
{
	SetOrthoScreen();
	gScene.Render2d(gViewer.GetGameCamera());
	//gViewer.m_pMode->Render2d();

	//RenderTerrainStreamProgress(0);

	if (gEditorEnabled)
		Editor_Render2d();
}