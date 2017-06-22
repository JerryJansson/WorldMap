#pragma once
#include "../../../../Source/Modules/GameObject/Entity.h"
//-----------------------------------------------------------------------------
class TileManager
{
public:
	//Hash<Vec3i, class MyTile*> m_Tiles;
	bool m_Initialized = false;
	Vec2d m_WorldOrigo;
	Vec3i m_LastTile;
	struct TileNode* m_RootNode;

public:
	TileManager() : m_RootNode(NULL) {}

	Vec2d GlToMercator2d(const Vec3& gl)
	{
		return Vec2d(m_WorldOrigo.x + gl.x, m_WorldOrigo.y - gl.z);	// going -z in gl is going "up" the globe (+y)
	}
	Vec3d GlToMercator3d(const Vec3& gl)
	{
		return Vec3d(m_WorldOrigo.x + gl.x, m_WorldOrigo.y - gl.z, gl.y);	// going -z in gl is going "up" the globe (+y)
	}
	Vec3 MercatorToGl(const Vec2d& m, float y=0)
	{
		double x = m.x - m_WorldOrigo.x;
		double z = -(m.y - m_WorldOrigo.y);							// going "up" the globe (+y) is going -z in gl		
		return Vec3(x, y, z);
	}

	void Update(CCamera* cam);
	//void UpdateRegularGrid(CCamera* cam);
	void UpdateSingleTile(const Vec3i& tms);
	void UpdateQTree(CCamera* cam);
	void Stop();

	void RenderDebug2d(CCamera* cam	);

private:
	void	Initialize(CCamera* cam);
	Vec3i	ShiftOrigo(CCamera* cam);
	bool	ReceiveLoadedTile();
};

extern TileManager gTileManager;