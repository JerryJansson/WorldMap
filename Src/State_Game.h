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
/*class TileHasher
{
public:
	static int Hash(const MyTile& tile)
	{
		return tile.grid.x ^ tile.grid.y ^ tile.grid.z;
	}
};*/
//-----------------------------------------------------------------------------
class TileManager
{
public:
	Hash<Vec3i, class MyTile*> m_Tiles;
	bool m_Initialized = false;
	Vec2d m_LongLat;
	Vec3d m_WorldOrigo;
	float m_TileSize;
	Vec3i m_LastTile;

	void Update(CCamera* cam);

};

extern TileManager gTileManager;