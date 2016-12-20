#include "Precompiled.h"
#include "UI.h"
#include "../../../Source/Modules/Shared/SceneNew.h"
#include "../../../Source/Modules/Shared/State.h"
#include "../../../Source/Modules/Landscape/LandscapeEnvironment.h"
#include "State_Game.h"
#include "Worldmap.h"
#include "vectiler\projection.h"

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

	gTileManager.Update(gViewer.GetGameCamera());
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





#define ZZZOOM 16
//-----------------------------------------------------------------------------
void TileManager::Update(CCamera* cam)
{
	if (!m_Initialized)
	{
		m_Initialized = true;
		m_LongLat = Vec2d(18.080, 59.346);	// Stockholm stadion

		Vec2d meters = lonLatToMeters(m_LongLat);
		Vec2i tile = MetersToTile(meters, ZZZOOM);

		//Vec4d tileBounds = TileBounds2(tile.x, tile.y, ZZZOOM);
		Vec4d tileBounds = TileBoundsInMeters(tile, ZZZOOM);

		Vec2d tileMin = tileBounds.xy();
		Vec2d tileMax = tileBounds.zw();
		m_TileSize = tileMax.x - tileMin.x;
		// Set world origo at corner of tile
		m_WorldOrigo.x = tileMin.x;
		m_WorldOrigo.y = 0.0f;
		m_WorldOrigo.z = tileMin.y;

		// Set camera in center of tile
		cam->SetWorldPos(Vec3(m_TileSize*0.5f, 20.0f, m_TileSize*0.5f));

		m_LastTile = Vec3i(0, 0, -1);
	}

	// Which tile is the camera in?
	/*Vec2d mercatorMeters = m_WorldOrigo.xz();
	mercatorMeters.x += cam->GetWorldPos().x;
	mercatorMeters.y += cam->GetWorldPos().z;
	Vec2i _camtile = MetersToTile(mercatorMeters, ZZZOOM);
	Vec3i camtile;
	camtile.x = _camtile.x;
	camtile.y = _camtile.y;
	camtile.z = ZZZOOM;

	const uint32 frame = Engine_GetFrameNumber();

	//if (m_LastTile != camtile)
	{
		const int range = 0;
		for (int ty = -range; ty <= range; ty++)
		{
			for (int tx = -range; tx <= range; tx++)
			{
				Vec3i t(camtile.x + tx, camtile.y + ty, camtile.z);
				MyTile* tile = m_Tiles.Get(t);

				// Cached
				if (tile)
				{
					// New origo
					if (m_LastTile != camtile)
					{
						float xx = m_TileSize * (tx + 0.5f);
						float zz = m_TileSize * (ty + 0.5f);
						tile->SetPos(xx, 10.0f, zz);
					}
				}
				else
				{
					tile = GetTile2(tx, ty, ZZZOOM);
					if (tile)
					{
						float xx = m_TileSize * (tx + 0.5f);
						float zz = m_TileSize * (ty + 0.5f);
						tile->SetPos(xx, 10.0f, zz);
					}
				}

				if (tile)
				{
					tile->m_Frame = frame;
				}
			}
		}

		m_LastTile = camtile;
	}

	// Release tiles
	for (int i = 0; i < m_Tiles.Num();)
	{
		MyTile* tile = m_Tiles[i].val;

		if (tile->m_Frame == frame)
		{
			i++;
			continue;
		}

		m_Tiles.Remove(tile->m_Grid);
		delete tile;
	}*/

	for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			Caabb a;
			a.m_Min = CVec3(x*m_TileSize, 0, y*m_TileSize);
			a.m_Max = CVec3((x+1)*m_TileSize, 10, (y+1)*m_TileSize);
			DrawWireAabb3d(a, gRGBA_Red);
			DrawWireCube(CVec3(x*m_TileSize*0.5f, 1.0f, y*m_TileSize*0.5f), 1, gRGBA_Red);
		}
	}
	//CVec3 pos = cam->GetWorldPos();
}

TileManager gTileManager;