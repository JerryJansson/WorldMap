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
Entity* CreateTestObject(const char* name, CVec3 pos, GGeom* geom)
{
	Entity* o = new Entity(name);
	o->SetPos(pos);
	MeshComponent* drawable = o->CreateComponent < MeshComponent >();
	//LoadMeshOptions mo;
	//mo.calculateTangentSpace = true;
	CMesh* mesh = CreateMeshFromGeoms(name, geom, 1);
	drawable->SetMesh(mesh, MeshComponent::eMeshDelete);
	
	return o;
}
//-----------------------------------------------------------------------------
void CreateTestObjects()
{
	root = new Entity("Test Objects");

	Caabb aabbs[3];
	GGeom geoms[3];
	CGeomGen::GenBox(&geoms[0], CVec3(1, 1, 1));
	CGeomGen::GenSphere_SubDiv(&geoms[1], 1.0f, CGeomGen::ICOSAHEDRON, 3);
	CGeomGen::GenCylinder(&geoms[2], 2.0f, 0.5f, 0.5f, 24, true);
	for (int i = 0; i < 3; i++)
		aabbs[i] = geoms[i].CalcAabb();

	for (int y = 0; y < 8; y++)
	{
		const char* materialname = NULL;
		if (y == 0) materialname = "testUnlit";
		if (y == 1) materialname = "testUnlitTex";
		if (y == 2) materialname = "testDiffuse";
		if (y == 3) materialname = "testDiffuseTex";
		if (y == 4) materialname = "testDiffuseSpec";
		if (y == 5) materialname = "testDiffuseTexSpec";
		if (y == 6) materialname = "testBump";
		if (y == 7) materialname = "testBumpSpec";

		for (int x = 0; x < 3; x++)
		{
			CVec3 pos(-10 + 8*x, 35, 230 - y*8);
			CStr name = Str_Printf("%s %d", x == 0 ? "Cube" : x == 1 ? "Sphere" : "Cylinder", y);
			
			geoms[x].SetMaterial(materialname);
			Entity* o = CreateTestObject(name, pos, &geoms[x]);
			root->AddChild(o);
		}
	}

	gScene.AddEntity(root);
}
//-----------------------------------------------------------------------------
void DestroyTestObjects()
{
	SAFE_DELETE(root);
}
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
	m_DebugCam.PerspectiveY(50.0f, 0.5f, 5000.0f);
	m_DefaultGameCam.SetName("Game Cam");
	m_DebugCam.SetName("Debug Cam");

	SetCamera(&m_DefaultGameCam);

	gScene.AddEntity(&m_DefaultGameCam);
	gScene.AddEntity(&m_DebugCam);
	
	CreateTestObjects();

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
	DestroyTestObjects();
	gScene.RemoveEntity(&m_DebugCam);
	gScene.RemoveEntity(&m_DefaultGameCam);
}
//-----------------------------------------------------------------------------
/*bool LoadObj(const char* fname)
{
	std::vector<tinyobj::material_t> materials;
	//std::map<std::string, GLuint>& textures;
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fname, NULL, true);
	if (!ret)
		return false;

	int nv = (int)(attrib.vertices.size()) / 3;
	int nn = (int)(attrib.normals.size()) / 3;
	int ns = (int)shapes.size();
	LOG("# of vertices  = %d\n", nv);
	LOG("# of normals   = %d\n", nn);
	LOG("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
	LOG("# of materials = %d\n", (int)materials.size());
	LOG("# of shapes    = %d\n", ns);

	assert(nv == nn);
	//assert(ns == 1);

	for (int s = 0; s < ns; s++)
	{
		tinyobj::shape_t& shape = shapes[s];
		tinyobj::mesh_t& m = shape.mesh;
		std::vector<tinyobj::index_t>& indices = m.indices;
		const int numTris = indices.size() / 3;
#if 1
		GGeom g;
		g.Prepare(nv, indices.size());
		GVertex* vtx = g.VertexPtr();
		uint32* idxs = g.IndexPtr();
		
		for (int f = 0; f < numTris; f++)
		{
			tinyobj::index_t idx0 = indices[3 * f + 0];
			tinyobj::index_t idx1 = indices[3 * f + 1];
			tinyobj::index_t idx2 = indices[3 * f + 2];
			idxs[f * 3 + 0] = idx0.vertex_index;
			idxs[f * 3 + 1] = idx1.vertex_index;
			idxs[f * 3 + 2] = idx2.vertex_index;
			assert(idx0.vertex_index == idx0.normal_index);
			assert(idx1.vertex_index == idx1.normal_index);
			assert(idx2.vertex_index == idx2.normal_index);
		}
		
		for (int iv = 0; iv < nv; iv++)
		{
			vtx[iv].pos.x = attrib.vertices[iv * 3 + 0];
			vtx[iv].pos.y = attrib.vertices[iv * 3 + 1];
			vtx[iv].pos.z = attrib.vertices[iv * 3 + 2];
			vtx[iv].nrm.x = attrib.normals[iv * 3 + 0];
			vtx[iv].nrm.y = attrib.normals[iv * 3 + 1];
			vtx[iv].nrm.z = attrib.normals[iv * 3 + 2];
		}
#else
		GGeom g;
		g.Prepare(0, indices.size());
		TArray<GVertex>& vtxArr = g.vertices;
		uint32* idxs = g.IndexPtr();

		// Ugly hack
		int voffs = indices[0].vertex_index;
		
		for (int f = 0; f < numTris; f++)
		{
			for (int k = 0; k < 3; k++)
			{
				// Index in obj file global vertex list
				int gi = indices[3 * f + k].vertex_index;
				// Local index in our local vertex array
				int li = gi - voffs;
				if (li >= vtxArr.Num())
				{
					vtxArr.SetNum(li + 1);
					GVertex& v = vtxArr[li];// vtxArr.AddEmpty();
					v.pos.x = attrib.vertices[gi * 3 + 0];
					v.pos.y = attrib.vertices[gi * 3 + 1];
					v.pos.z = attrib.vertices[gi * 3 + 2];
					v.nrm.x = attrib.normals[gi * 3 + 0];
					v.nrm.y = attrib.normals[gi * 3 + 1];
					v.nrm.z = attrib.normals[gi * 3 + 2];
				}
				idxs[3 * f + k] = li;
			}
		}

		int maxv = g.vertices.Num() - 1;
		for (int i = 0; i < g.indexes.Num(); i++)
		{
			if (g.indexes[i] > maxv)
			{
				int abba = 10;
			}
		}
#endif

		g.SetMaterial("testDiffuseSpec");

		Entity* entity = new Entity(shape.name.c_str());
		MeshComponent* meshcomp = entity->CreateComponent<MeshComponent>();
		meshcomp->m_DrawableFlags.Set(Drawable::eLightMap);

		CMesh* mesh = CreateMeshFromGeoms(shape.name.c_str(), &g, 1);
		meshcomp->SetMesh(mesh, MeshComponent::eMeshDelete);

		entity->SetPos(CVec3(0,10,-10));
		gScene.AddEntity(entity);
	}	

	return true;
}*/
//-----------------------------------------------------------------------------
int gMaterialCategory = -1;
//-----------------------------------------------------------------------------
bool MyApp_Init()
{
	Input_SetMouseMode(MOUSE_MODE_HIDDEN);
	Editor_Init();
	
	if (!gScene.Create(NULL))
		return false;

	gMaterialCategory = gResourceFactory.LoadMaterialLib("content_worldmap/materials.smt");

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
	gResourceFactory.RemoveMaterialLib(gMaterialCategory);
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