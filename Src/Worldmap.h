#pragma once

//-----------------------------------------------------------------------------
#include "../../../Source/Modules/CamFly.h"
//-----------------------------------------------------------------------------
//enum ESimviewMode { eFly, eDetailEdit };
//extern const char* simviewModeNames[];
//-----------------------------------------------------------------------------
class CViewer
{
private:
	CCamera_Fly	 m_DefaultGameCam;
	CCamera_Fly	 m_DebugCam;

	CCamera*	 m_ActiveGameCam;	// Usually points to m_DefaultGameCam, but can be set to another camera (See mode DetailEdit)

	CCamera*	m_DrawCamera;		// Points to the camera we are looking from
	CCamera*	m_InputCamera;		// Points to the camera we are controlling

public:
	//ESimviewMode		m_Mode;
	//class CSimViewMode*	m_pMode;

public:
	CViewer();
	~CViewer(){};

	bool Create();
	void Destroy();
	bool Init();
	void Deinit();
	
	void SetCamera(CCamera* cam)
	{
		m_ActiveGameCam = cam;
		SetInputCamera(cam);
		SetDrawCamera(cam);
	}
	void SetDrawCamera(CCamera* cam)
	{
		m_DrawCamera = cam;
	}
	void SetInputCamera(CCamera* cam)
	{
		if (m_InputCamera)
			m_InputCamera->SetFlag(Entity::eInput, false);
		if (cam)
			cam->SetFlag(Entity::eInput, true);
		
		m_InputCamera = cam;
	}
	void ToggleDebugCamera(bool input)
	{
		if (input)
		{
			if (m_InputCamera == m_ActiveGameCam) SetInputCamera(&m_DebugCam);
			else								 SetInputCamera(m_ActiveGameCam);
		}
		else
		{
			if (m_DrawCamera == m_ActiveGameCam)
			{
				SetDrawCamera(&m_DebugCam);
				SetInputCamera(&m_DebugCam);
			}
			else
			{
				SetDrawCamera(m_ActiveGameCam);
				SetInputCamera(m_ActiveGameCam);
			}
		}
	}

	CCamera_Fly*	GetDefaultCamera()		{ return &m_DefaultGameCam; }
	CCamera*		GetGameCamera()			{ return m_ActiveGameCam; }
	CCamera_Fly*	GetDebugCamera()		{ return &m_DebugCam;	 }
	CCamera*		GetInputCamera()		{ return m_InputCamera; }
	CCamera*		GetDrawCamera()		{ return m_DrawCamera; }
//	void			SetMode(ESimviewMode mode);//const char* strMode)
};
//-----------------------------------------------------------------------------
extern CViewer gViewer;
//-----------------------------------------------------------------------------
class CSimViewMode
{
public:

public:
	CSimViewMode() {};
	virtual ~CSimViewMode() {};

	virtual bool Init()					{ return true; }
	virtual void Deinit()				{}
	virtual void Update(const float dt)	{}
	virtual void Input()				{}
	virtual void Render2d()				{}
	virtual void Render3d()				{}
};