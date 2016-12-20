#include "Precompiled.h"
#include "../../../Source/Modules/Shared/SceneNew.h"
#include "../../../Source/Modules/Shared/State.h"
#include "../../../Source/Modules/Landscape/LandscapeEnvironment.h"
#include "../../../Source/Modules/Terrain/Terrain.h"
#include "State_Loading.h"
#include "State_Game.h"
#include "SimView.h"
//-----------------------------------------------------------------------------
CStrL gStrCourse;
//CStrL gStrCourse = "courses/pt_tg_baseball/course.xml";
//CStrL gStrCourse = "courses/protracer_range/course.xml";
//CStrL gStrCourse = "courses/pt_kiawah_ocean/course.xml";
//CStrL gStrCourse = "courses/pt_royaltroon/course.xml";
//CStrL gStrCourse = "courses/brohof_13/course.xml";
//CStrL gStrCourse = "courses/protracer_flat/course.xml";
//CStrL gStrCourse = "courses/protracer_london_gc/course.xml";
//CStrL gStrCourse = "courses/newmalden_range/course.xml";
//CStrL gStrCourse = "courses/soderby/course.xml";
//CStrL gStrCourse = "courses/ostersund/course.xml";
//CStrL gStrCourse = "courses/os_rowing_10k/course.xml";
//CStrL gStrCourse = "courses/pt_mcinnis/course.xml";

const bool instant_start = true;
//-----------------------------------------------------------------------------
bool CState_Loading::PreLoadCourse()
{
	CVec3 initPos;
	if(!gEnvironment.PreLoad(gStrCourse, &initPos))
		return false;

	//initPos = CVec3(525, 50, 87);

	// Setup cameras
	gViewer.GetGameCamera()->SetLocalPos(initPos);
	gViewer.GetDebugCamera()->LookAt(initPos + CVec3(50,50,50), initPos, CVec3(0,1,0));

	return true;
}
//-----------------------------------------------------------------------------
bool CState_Loading::Init()
{
	if(!CState::Init())
		return false;

	m_pGui->SetFrameText("Progress", "Preloading");
	m_LoadState=0;
	return true;
}
//-----------------------------------------------------------------------------
void CState_Loading::Update(const float dt)
{
	CState::Update(dt);

	// An empty state just to bring up the load screen instantly
	if(m_LoadState==0)
	{
        if(!m_pManager->IsFading())
            m_LoadState++;
	}
	else if(m_LoadState==1)
	{
		if(PreLoadCourse())
		{
			m_LoadState++;
		}
		else
		{
			CStrL msg = Str_Printf("Failed to load course \"%s\"", gStrCourse.Str());
			m_pGui->SetFrameText("Progress", msg);
			m_LoadState = 99;
		}
	}
	else if(m_LoadState==2)
	{
		if(instant_start)
		{
			m_pManager->SetActiveState("Game");
			m_LoadState++;
		}
		else
		{
			float complete = gTerrain.Update(gViewer.GetGameCamera());
			m_pGui->SetFrameText("Progress", Str_Printf("%d%%", (int)(complete * 100)));
			if(complete>=1.0f)
			{
				m_pManager->SetActiveState("Game");
				m_LoadState++;
			}
		}
	}
}
//-----------------------------------------------------------------------------
void CState_Loading::Render2d()
{
	gRenderer->ClearBuffers(CLR_COLOR|CLR_DEPTH);
	CState::Render2d();
}

//-----------------------------------------------------------------------------
//
//	MAIN MENU
//
//-----------------------------------------------------------------------------
bool CState_MainMenu::Init()
{
	if(!CState::Init())
		return false;

	//if (gStrCourse != "")
	//	Unload();

	Input_SetMouseMode(MOUSE_MODE_HIDDEN);

	CGuiFrame_List* courseList = (CGuiFrame_List*)m_pGui->GetFrame("CourseList", FRM_LIST);

	ListCourses(m_Courses);

	for (int i = 0; i<m_Courses.Num(); i++)
	{
		CFile f;
		CStrL testNewEngine = Str_Printf("courses/%s/objectset2.xml", m_Courses[i].courseDir.Str());
		if (!f.Open(testNewEngine, FILE_READ))	continue;

		CGuiFrame* item = courseList->AddItem();
		item->SetText(m_Courses[i].courseName);
		item->m_UsrPtr = &m_Courses[i];
	}

	courseList->SetSelectedItem(0);

	return true;
}
//-----------------------------------------------------------------------------
void CState_MainMenu::Update(const float dt)
{
	m_pGui->Update(dt);

	CGuiMsg* msg;
	while( (msg=m_pGui->GetMsg()) )
	{
		if(msg->m_Msg == GMSG_CLICKED)
		{
			if(msg->m_Frame->GetName()=="Start")
			{
				CGuiFrame_List* courseList = (CGuiFrame_List*)m_pGui->GetFrame("CourseList", FRM_LIST);
				CourseInfo* c = (CourseInfo*)courseList->GetSelectedItem()->m_UsrPtr;
				gStrCourse = Str_Printf("courses/%s/%s", c->courseDir.Str(), c->courseXml.Str());
				m_pManager->SetActiveState("Load");
			}
		}
	}
}
//-----------------------------------------------------------------------------
void CState_MainMenu::Render2d()
{
	gRenderer->ClearBuffers(CLR_COLOR|CLR_DEPTH);
	CState::Render2d();
}