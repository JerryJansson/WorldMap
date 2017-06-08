#include "Precompiled.h"
#include "../../../Source/Middleware/ImGui/imgui_helper.h"
#include "../../../Source/Modules/Landscape/LandscapeEnvironment.h"
//-----------------------------------------------------------------------------
void Editor_Init()
{
	gEnvironment.m_EditorMode = true;
	ImHelper_Init();
}
//-----------------------------------------------------------------------------
void Editor_Deinit()
{
	ImHelper_Deinit();
}
//-----------------------------------------------------------------------------
bool Editor_Update(float dt/*, bool allowGuiInput*/)
{
	ImHelperIO io;
	ImHelper_NewFrame(&io);

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			//if (ImGui::MenuItem("Save", "Ctrl+S")) Save();
			//if (ImGui::MenuItem("Exit", "Ctrl+X")) E_Quit();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
			if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
			ImGui::Separator();
			if (ImGui::MenuItem("Cut", "CTRL+X")) {}
			if (ImGui::MenuItem("Copy", "CTRL+C")) {}
			if (ImGui::MenuItem("Paste", "CTRL+V")) {}
			ImGui::EndMenu();
		}
		/*if (ImGui::BeginMenu("Window"))
		{
			ImGui::MenuItem("Environment", NULL, &show_wdw_environment);
			ImGui::MenuItem("CVar", NULL, &show_wdw_cvar);
			ImGui::MenuItem("Compress Textures", NULL, &show_wdw_texcomp);
			ImGui::EndMenu();
		}*/

		ImGui::EndMainMenuBar();
	}

	/*if (show_wdw_environment)	ImWindow_Environment();
	if (show_wdw_cvar)			ImWindow_CVar();
	if (show_wdw_texcomp)		ImWindow_TextureCompressor();
	ImWindow_ContentBrowser();
	ImWindow_Scene();
	ImWindow_Inspector();*/

	//return inputHandled;
	return false;
}
//-----------------------------------------------------------------------------
void Editor_Render2d()
{
	int w, h; gRenderer->GetScreenSize(w, h);
	gMvp.Ortho(0, w, 0, h, 0, 100); // Identity Model & View
	ImGui::Render();

	// Safe to relese texture after ImGui::Render()
	//if (postReleaseTexture)
	//	gResourceFactory.ReleaseTexture(&postReleaseTexture);
}