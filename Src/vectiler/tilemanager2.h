#pragma once
#include "../../../../Source/Modules/GameObject/Entity.h"
//-----------------------------------------------------------------------------
class MyTile : public Entity
{
public:
	enum EStatus { eNotLoaded, eLoaded, eError } m_Status;
public:
	Vec3i	m_Tms;		// Tms grid coord xyz (z=zoom)
	Vec2d	m_Origo;	// Tile origo (lower left corner) in mercator coordinates
	uint32	m_Frame;	// Last frame we needed this tile

	MyTile(const Vec3i& tms);
	EStatus Status() const { return m_Status; }
};
//-----------------------------------------------------------------------------
struct TileData
{
	enum Status 
	{
		eNotLoaded,
		eLoading,
		eLoaded,
		eError
	};
	
	Status				m_Status;
	Vec3i				m_Tms;				// Tms grid coord xyz (z=zoom)
	uint32				m_FrameBump;		// Last frame we used this data
	struct TileNode*	m_Node;

	bool IsLoaded() const { return m_Status == TileData::eLoaded; }
};
//-----------------------------------------------------------------------------
struct TileNode
{
	Vec3i		m_Tms;		// Tms grid coord xyz (z=zoom)
	TileNode*	m_Child[4];
	TileNode*	m_Parent;
	float		m_Dist;
	TileData*	m_Data;
	Caabb		m_NodeAabb;

	TileNode() {}
	bool IsLoaded() const					{ return m_Data && m_Data->IsLoaded(); }
	//bool IsLoading() const				{ return m_Data && m_Data->m_Status == TileData::eLoading; }
	//void FrameBump(const uint32 frame)	{ if (m_Data) m_Data->m_LastUsed = frame; }
};
//-----------------------------------------------------------------------------
class TileManager
{
public:
	Hash<Vec3i, class MyTile*> m_Tiles;
	bool m_Initialized = false;
	Vec2d m_WorldOrigo;
	Vec3i m_LastTile;

	// New
	TileNode* m_RootNode;

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
	void UpdateRegularGrid(CCamera* cam);
	void UpdateSingleTile(const Vec3i& tms);
	void UpdateQTree(CCamera* cam);
	void Stop();

	void RenderDebug2d(CCamera* cam	);

private:
	void	Initialize(CCamera* cam);
	Vec3i	ShiftOrigo(CCamera* cam);
	//int		ChooseTileToLoad(CCamera* cam, const TArray<Vec3i>& tiles);
	bool	ReceiveLoadedTile();
};

extern TileManager gTileManager;