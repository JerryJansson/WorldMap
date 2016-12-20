#pragma once
//-----------------------------------------------------------------------------
class CState_Game : public CState
{

public:
	CState_Game(const char* name, const char* gui_fname) : CState(name,gui_fname){};
	virtual bool Init();
	virtual void Deinit();
	virtual void Update(const float dt);
	virtual void Render();
	virtual void Render2d();
	virtual bool ProcessInput(const inputEvent_t* e);
};