#pragma once
//-----------------------------------------------------------------------------
#include "../../../Source/Modules/Landscape/LandscapeEnvironment.h"
//-----------------------------------------------------------------------------
class CState_MainMenu : public CState
{
public:
	TArray<CourseInfo> m_Courses;

public:
	CState_MainMenu(const char* name, const char* gui_fname) : CState(name, gui_fname) {};
	virtual bool Init();
	virtual void Update(const float dt);
	virtual void Render2d();
};
//-----------------------------------------------------------------------------
class CState_Loading : public CState
{
private:
	uint8 m_LoadState;

public:
	CState_Loading(const char* name, const char* gui_fname) : CState(name, gui_fname) {};
	virtual bool Init();
	virtual void Update(const float dt);
	virtual void Render2d();

private:
	bool PreLoadCourse();
};