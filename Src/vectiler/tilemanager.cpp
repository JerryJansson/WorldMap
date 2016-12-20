#include "Precompiled.h"
#include "tilemanager.h"
#include "projection.h"
#include "jerry.h"
#include "../../../../Source/Modules/GameObject/Camera.h"
#include "../../../../Source/Modules/Shared/SceneNew.h"
#include <thread>
#include <mutex>
//-----------------------------------------------------------------------------
static CVar tile_QuadTree("tile_QuadTree", true);
//-----------------------------------------------------------------------------
//#define ZZZOOM 16
#define ZZZOOM 16						// Regular grid uses a fixed zoom
const bool useSingleTile = false;		// Only load 1 tile. Good for debugging
//const Vec3i singleTile(1204, 2554, 12);	// Contains a mesh > 65536 vertices
const Vec3i singleTile(4820, 10224, 14);	// Contains a mesh == 65536 vertices
//const Vec3i singleTile(19294, 40893, 16);
//const Vec3i singleTile(9647, 20446, 15);
//-----------------------------------------------------------------------------
// Threads & synchronization
//-----------------------------------------------------------------------------
const int numThreads = 1;
std::thread threads[numThreads];
bool stop = false;
std::mutex tileLock;
std::mutex poolLock;
std::mutex doneLock;
MyTile* tileToStream = NULL;
//-----------------------------------------------------------------------------
struct StreamResult : ListNode<StreamResult>
{
	MyTile* tile;
	TArray<GGeom> geoms;
};
MemPoolDynamic<StreamResult> geomPool(8);
List2<StreamResult> doneList;
//-----------------------------------------------------------------------------
MyTile::MyTile(const Vec3i& tms)
{
	m_Frame  = 0;
	m_Tms	 = tms;
	m_Origo  = TileMin(m_Tms);
	m_Status = eNotLoaded;
	
	const CStrL tileName = Str_Printf("%d_%d_%d", tms.x, tms.y, tms.z);
	SetName(tileName);
}
//-----------------------------------------------------------------------------
void StreamTile(int threadIdx)
{
	while (!stop)
	{
		Sleep(10);

		if (!tileToStream)			continue;
		if (!tileLock.try_lock())	continue;
		if (!tileToStream)
		{
			tileLock.unlock();
			continue;
		}

		LOG("Thread %d picked up <%d, %d> in frame %d\n", threadIdx, tileToStream->m_Tms.x, tileToStream->m_Tms.y, Engine_GetFrameNumber());

		MyTile* t = tileToStream;
		tileToStream = NULL;
		tileLock.unlock();

		poolLock.lock();
		StreamResult* result = geomPool.Alloc();
		poolLock.unlock();

		result->tile = t;
		GetTile(result->tile, result->geoms);

		doneLock.lock();
		doneList.PushBack(result);
		doneLock.unlock();
	}
}
//-----------------------------------------------------------------------------
void TileManager::Stop()
{
	stop = true;

	// Wait for all streaming threads
	for (int i = 0; i < numThreads; i++)
		threads[i].join();

	// Receive all streamed tiles
	while (ReceiveLoadedTile()) {}

	/*for (int i = 0; i < m_Tiles.Num(); i++)
	{
	MyTile* t = m_Tiles[i];
	gScene.RemoveEntity(t);
	m_Tiles.Remove
	}*/
}
//-----------------------------------------------------------------------------
void TileManager::Initialize(CCamera* cam)
{
	m_Initialized = true;

	Vec3i tile;
	if (useSingleTile) // Debug
	{
		tile = singleTile;
	}
	else
	{
		//m_LongLat = Vec2d(18.080, 59.346);		// 36059, 19267 - Stockholm Stadion
		Vec2d longLat = Vec2d(-74.0130, 40.703);	// 19294, 24642 - Manhattan

		Vec2d meters = LonLatToMeters(longLat);
		Vec2i _tile = MetersToTile(meters, ZZZOOM);
		tile = Vec3i(_tile.x, _tile.y, ZZZOOM);
	}
	
	Vec2d tileMin = TileMin(tile);
	Vec2d tileMax = TileMax(tile);
	double size = tileMax.x - tileMin.x;
	
	// Set world origo at corner of tile
	m_WorldOrigo = tileMin;

	// Set camera in center of tile
	cam->SetWorldPos(Vec3(size*0.5f, 40.0f, -size*0.5f));

	m_LastTile = Vec3i(0, 0, -1);

	for (int i = 0; i < numThreads; i++)
		threads[i] = std::thread(StreamTile, 0);
}
//-----------------------------------------------------------------------------
Vec3i TileManager::ShiftOrigo(CCamera* cam)
{
	// Which tile is the camera in?
	const Vec2d camMercator = GlToMercator2d(cam->GetWorldPos());
	const Vec2i _camtile = MetersToTile(camMercator, ZZZOOM);
	const Vec3i camtile = Vec3i(_camtile.x, _camtile.y, ZZZOOM);

	// We changed tile. The new tile is the new center of the world. Make camera position relative the
	// new world origo
	if (m_LastTile != camtile)
	{
		m_WorldOrigo = TileMin(_camtile, ZZZOOM);
		Vec3 pos = MercatorToGl(camMercator);
		pos.y = cam->GetWorldPos().y;
		cam->SetPos(pos);

		// Relocate tiles relative new origo
		for (int i = 0; i < m_Tiles.Num(); i++)
		{
			MyTile* tile = m_Tiles[i].val;
			tile->SetPos(MercatorToGl(tile->m_Origo, 5.0f));
		}

		m_LastTile = camtile;
	}

	return camtile;
}
//-----------------------------------------------------------------------------
// Prio 1: In frustum
// Prio 2: Distance from camera
//-----------------------------------------------------------------------------
int TileManager::ChooseTileToLoad(CCamera* cam, const TArray<Vec3i>& tiles)
{
	const Vec3 camGlPos = cam->GetWorldPos();
	const CFrustum& f = cam->GetFrustum();

	float mindist = -1;
	bool minvis = false;
	int minidx = -1;

	for (int i = 0; i < tiles.Num(); i++)
	{
		const Vec3i& t = tiles[i];
		const Vec2d tileMin = TileMin(t);
		const Vec2d tileMax = TileMax(t);
		const float size = (tileMax.x - tileMin.x);
		Caabb aabb(MercatorToGl(tileMin, 0.0f), MercatorToGl(tileMax, 1000.0f));
		Swap(aabb.m_Min.z, aabb.m_Max.z);

		if (BoxIntersectsFrustum(f, aabb))
		{
			const float dist = DistancePointAabb(camGlPos, aabb);
			if (dist < mindist || minidx == -1 || minvis==false)
			{
				minvis = true;
				minidx = i;
				mindist = dist;
			}
		}
		else if (minvis == false)
		{
			const float dist = DistancePointAabb(camGlPos, aabb);
			if (dist < mindist || minidx == -1)
			{
				minidx = i;
				mindist = dist;
			}
		}
	}

	return minidx;
}
//-----------------------------------------------------------------------------
bool TileManager::ReceiveLoadedTile()
{
	// No tiles waiting to be received
	if (doneList.IsEmpty())
		return false;

	StreamResult* r = NULL;
	if (doneLock.try_lock())
	{
		r = doneList.PopFront();
		doneLock.unlock();
	}

	if (!r)
		return false;

	MyTile* t = r->tile;
	LOG("Received %d, %d in frame %d\n", t->m_Tms.x, t->m_Tms.y, Engine_GetFrameNumber());
	t->m_Status = MyTile::eLoaded;
	t->m_Frame = Engine_GetFrameNumber();

	for (int i = 0; i < r->geoms.Num(); i++)
	{
		CMesh* mesh = CreateMeshFromGeoms(r->geoms[i].name, &r->geoms[i], 1);
		Entity* e = new Entity(r->geoms[i].name);
		MeshComponent* meshcomp = e->CreateComponent<MeshComponent>();
		meshcomp->m_DrawableFlags.Set(Drawable::eLightMap);
		meshcomp->SetMesh(mesh, MeshComponent::eMeshDelete); // Resets dirty flag
		t->AddChild(e);
	}

	t->SetPos(MercatorToGl(t->m_Origo, 5.0f)); // Must set position here, after children are added. Because children need their transform dirtied

	gScene.AddEntity(t);

	r->tile = NULL;
	// Release geom memory
	poolLock.lock();
	geomPool.Free(r);
	poolLock.unlock();

	return true;
}
//-----------------------------------------------------------------------------
static TArray<Vec3i> neededTiles(256);
//-----------------------------------------------------------------------------
void TileManager::Update(CCamera* cam)
{
	if (!m_Initialized)
		Initialize(cam);

	ShiftOrigo(cam);

	// Determine needed tiles
	neededTiles.Clear();
	if (useSingleTile)
	{
		neededTiles.Add(singleTile);
	}
	else
	{
		if (tile_QuadTree)	UpdateQTree(cam);
		else				UpdateRegularGrid(cam);
	}

	// Determine missing tiles
	const uint32 frame = Engine_GetFrameNumber();
	static TArray<Vec3i> missingTiles(256);
	missingTiles.Clear();
	for (int i = 0; i < neededTiles.Num(); i++)
	{
		const Vec3i& t = neededTiles[i];
		MyTile* tile = m_Tiles.Get(t);
		if(tile) tile->m_Frame = frame;
		else	 missingTiles.Add(t);
	}

	DbgMsg("Tiles to load: %d", missingTiles.Num());

	// Add tile to queue to stream
	if (missingTiles.Num()>0 && !tileToStream)
	{
		int loadIdx = ChooseTileToLoad(cam, missingTiles);
		
		MyTile* newtile = NULL;
		if (tileLock.try_lock())	// No need to verify !tileToStream inside lock. Main thread is the only thread that can set tileToStream!=NULL
		{
			tileToStream = new MyTile(missingTiles[loadIdx]);
			newtile = tileToStream;
			LOG("Added %d,%d for streaming in frame %d\n", tileToStream->m_Tms.x, tileToStream->m_Tms.y, frame);
			tileLock.unlock();
		}
		
		if(newtile)
			m_Tiles.Add(newtile->m_Tms, newtile);
	}

	// Any tiles done streaming?
	ReceiveLoadedTile();
}

//-----------------------------------------------------------------------------
void TileManager::UpdateRegularGrid(CCamera* cam)
{
	// Which tile is the camera in?
	const Vec2d camMercator = GlToMercator2d(cam->GetWorldPos());
	const Vec2i _camtile = MetersToTile(camMercator, ZZZOOM);
	const Vec3i camtile = Vec3i(_camtile.x, _camtile.y, ZZZOOM);

	// range = 0 -> 1x1
	// range = 1 -> 3x3
	// range = 2 -> 5x5
	// range = 3 -> 7x7
	const int range = 1;
	const int maxTiles = (range*2 + 1) * (range*2+1);
	
	neededTiles.EnsureCapacity(maxTiles);

	for (int ty = -range; ty <= range; ty++)
	{
		for (int tx = -range; tx <= range; tx++)
		{
			const Vec3i t(camtile.x + tx, camtile.y + ty, camtile.z);
			neededTiles.Add(t);
		}
	}
}
//-----------------------------------------------------------------------------
/*inline double Distance_ToTile(const Vec3d &p, const Vec3d& mini, const Vec3d& maxi)
{
	double d;
	double dist = 0;

	for (int i = 0; i<3; i++)
	{
		if (p[i]<mini[i])
		{
			d = mini[i] - p[i];
			dist += d*d;
		}
		else if (p[i]>maxi[i])
		{
			d = p[i] - maxi[i];
			dist += d*d;
		}
	}

	return sqrt(dist);
}*/
//-----------------------------------------------------------------------------
/*void GetTileCullInfo(const Vec3& camGlPos, const Vec3i& idx, float& size, float& dist)
{
	const Vec2d tileMin = TileMin(idx);
	const Vec2d tileMax = TileMax(idx);
	size = tileMax.x - tileMin.x;
	Caabb aabb(MercatorToGl(tileMin, 0.0f), MercatorToGl(tileMax, 1000.0f));
	Swap(aabb.m_Mini.z, aabb.m_Max.z);
	
	const float dist = DistancePointAabb(camGlPos, aabb);
}*/
//-----------------------------------------------------------------------------
void TileManager::UpdateQTree(CCamera* cam)
{
	const uint32 frame	= Engine_GetFrameNumber();
	const Vec3 camGlPos = cam->GetWorldPos();
	const Vec3d camMercator3d = GlToMercator3d(camGlPos);
	const float maxDrawDist = 12 * 1000; // 12km
	const int maxZoom = 17;
	int n = 0;

	TStack<Vec3i, 48> stack(Vec3i(0, 0, 0));
	while (stack.Count())
	{
		n++;
		const Vec3i& t = stack.Pop();
		const Vec2d tileMin = TileMin(t);
		const Vec2d tileMax = TileMax(t);
		const float size = (tileMax.x - tileMin.x);
		Caabb aabb(MercatorToGl(tileMin, 0.0f), MercatorToGl(tileMax, 1000.0f));
		Swap(aabb.m_Min.z, aabb.m_Max.z);
		const float dist = DistancePointAabb(camGlPos, aabb);
		//const Vec3d mini = Vec3d(tileMin.x, tileMin.y, 0);
		//const Vec3d maxi = Vec3d(tileMax.x, tileMax.y, 1000.0f);
		//const double dist = Distance_ToTile(camMercator3d, mini, maxi);

		// To far away to be visible
		if (dist > maxDrawDist)
			continue;

		// Too far away to split further (or max zoom is reached)
		if (dist > size || t.z >= maxZoom)
		{
			neededTiles.Add(t);
		}
		// Close enough to split
		else
		{
			int zoom = t.z + 1;
			int x = t.x << 1;
			int y = t.y << 1;
			stack.Push(Vec3i(x, y, zoom));
			stack.Push(Vec3i(x + 1, y, zoom));
			stack.Push(Vec3i(x + 1, y + 1, zoom));
			stack.Push(Vec3i(x, y + 1, zoom));
		}
	}
}
//-----------------------------------------------------------------------------
void TileManager::RenderDebug2d(CCamera* cam)
{
	int sw = gRenderer->GetScreenWidth();
	int sh = gRenderer->GetScreenHeight();
	Vec2 screenpos(sw / 2, sh / 2);

	const Vec2d camMercator2d = GlToMercator2d(cam->GetWorldPos());

	int minz = 200;
	int maxz = 0;

	if (neededTiles.Num() < 100)
	{
		int abba = 10;
	}

	for (int i = 0; i < neededTiles.Num(); i++)
	{
		const Vec3i& t = neededTiles[i];
		minz = Min(minz, t.z);
		maxz = Max(maxz, t.z);

		Vec2d pmind = (TileMin(t) - camMercator2d) / 35.0;
		Vec2d pmaxd = (TileMax(t) - camMercator2d) / 35.0;
		Vec2 pmin = Vec2(pmind.x, pmind.y);
		Vec2 pmax = Vec2(pmaxd.x, pmaxd.y);
		pmin += screenpos;
		pmax += screenpos;
		DrawWireRect(pmin, pmax, gRGBA_Red);
	}

	DrawWireCircle(screenpos, 3.0f, gRGBA_Black);
	DbgMsg("zoom: %d-%d", minz, maxz);
}


TileManager gTileManager;