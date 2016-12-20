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

//-----------------------------------------------------------------------------
class TileManager
{
public:
	Hash<Vec3i, class MyTile*> m_Tiles;
	bool m_Initialized = false;
	Vec2d m_LongLat;
	//Vec3d m_WorldOrigo;
	Vec2d m_WorldOrigo;
	float m_TileSize;
	Vec3i m_LastTile;

	Vec2d GlToMercator(const Vec3& gl)
	{
		return Vec2d(m_WorldOrigo.x + gl.x, m_WorldOrigo.y - gl.z);	// going -z in gl is going "up" the globe (+y)
	}
	Vec3 MercatorToGl(const Vec2d& m)
	{
		double x = m.x - m_WorldOrigo.x;
		double z = -(m.y - m_WorldOrigo.y);							// going "up" the globe (+y) is going -z in gl		
		return Vec3(x, 0, z);
	}

	void Update(CCamera* cam);

};

extern TileManager gTileManager;