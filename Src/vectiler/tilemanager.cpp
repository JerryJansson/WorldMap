#include "Precompiled.h"
#include "tilemanager.h"
#include "projection.h"
#include "jerry.h"
#include "../../../../Source/Modules/GameObject/Camera.h"
#include "../../../../Source/Modules/Shared/SceneNew.h"
#include <thread>
#include <mutex>
//-----------------------------------------------------------------------------
static CVar tile_QuadTree("tile_QuadTree", false);
//-----------------------------------------------------------------------------
//#define ZZZOOM 16
#define ZZZOOM 16					// Regular grid uses a fixed zoom
const bool useSingleTile = false;	// Only load 1 tile. Good for debugging
const Vec3i singleTile(1204, 2554, 12);
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
		GetTile3(result->tile, result->geoms);

		doneLock.lock();
		doneList.PushBack(result);
		doneLock.unlock();
	}
}
//-----------------------------------------------------------------------------
void TileManager::Stop()
{
	stop = true;
	for (int i = 0; i < numThreads; i++)
		threads[i].join();
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
static TArray<Vec3i> neededTiles(256);
//-----------------------------------------------------------------------------
void TileManager::Update(CCamera* cam)
{
	if (!m_Initialized)
		Initialize(cam);

	const Vec3i camtile = ShiftOrigo(cam);

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

	// Add tile to queue to stream
	if (missingTiles.Num()>0 && !tileToStream)
	{
		MyTile* newtile = NULL;
		if (tileLock.try_lock())
		{
			if (!tileToStream)
			{
				tileToStream = new MyTile(missingTiles[0]);
				newtile = tileToStream;
			}
			LOG("Added %d,%d for streaming in frame %d\n", tileToStream->m_Tms.x, tileToStream->m_Tms.y, frame);
			tileLock.unlock();
		}
		
		if(newtile)
			m_Tiles.Add(newtile->m_Tms, newtile);
	}

	// Any tiles done streaming?
	if (!doneList.IsEmpty())
	{
		StreamResult* r = NULL;
		if (doneLock.try_lock())
		{
			r = doneList.PopFront();
			doneLock.unlock();
		}

		if (r)
		{
			MyTile* t = r->tile;
			LOG("Received %d, %d in frame %d\n", t->m_Tms.x, t->m_Tms.y, frame);
			t->m_Status = MyTile::eLoaded;
			t->m_Frame = frame;

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
		}
	}
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
	//static TArray<Vec3i> tilesToLoad;
	//tilesToLoad.Clear();
	neededTiles.EnsureCapacity(maxTiles);

	for (int ty = -range; ty <= range; ty++)
	{
		for (int tx = -range; tx <= range; tx++)
		{
			const Vec3i t(camtile.x + tx, camtile.y + ty, camtile.z);
			neededTiles.Add(t);
			/*MyTile* tile = m_Tiles.Get(t);

			if (!tile)
				tilesToLoad.Add(t);

			if (tile)
				tile->m_Frame = frame;*/
		}
	}

	/*for (int y = 0; y < 3; y++)
	{
		for (int x = 0; x < 3; x++)
		{
			Caabb a;
			a.m_Min = CVec3(x*m_TileSize, 0, y*m_TileSize);
			a.m_Max = CVec3((x + 1)*m_TileSize, 10, (y + 1)*m_TileSize);
			DrawWireAabb3d(a, gRGBA_Red);

			float center_x = ((float)x + 0.5f) * m_TileSize;
			float center_y = ((float)y + 0.5f) * m_TileSize;

			DrawWireCube(CVec3(center_x, 1.0f, center_y), 1, gRGBA_Red);
		}
	} */
}
//-----------------------------------------------------------------------------
inline double Distance_ToTile(const Vec3d &p, const Vec3d& mini, const Vec3d& maxi)
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
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void TileManager::UpdateQTree(CCamera* cam)
{
	const uint32 frame	= Engine_GetFrameNumber();
	const Vec3 camGlPos = cam->GetWorldPos();
	const Vec3d camMercator3d = GlToMercator3d(camGlPos);
	const double maxDrawDist = 12 * 1000; // 12km
	const double maxZoom = 17;

	
	neededTiles.Clear();
	
	int n = 0;

	TStack<Vec3i, 48> stack(Vec3i(0, 0, 0));
	while (stack.Count())
	{
		n++;
		const Vec3i& t = stack.Pop();
		//const Vec2i _camtile = MetersToTile(camMercator, ZZZOOM);
		const Vec2d tileMin = TileMin(t);
		const Vec2d tileMax = TileMax(t);
		const double size = (tileMax.x - tileMin.x);
		//const Vec3d mini = Vec3d(tileMin.x - rad*0.5, tileMin.y - rad*0.5, 0);
		//const Vec3d maxi = Vec3d(tileMax.x + rad*0.5, tileMax.y + rad*0.5, rad*2);
		const Vec3d mini = Vec3d(tileMin.x, tileMin.y, 0);
		const Vec3d maxi = Vec3d(tileMax.x, tileMax.y, size*2);
		const double dist = Distance_ToTile(camMercator3d, mini, maxi);

		// To far away to be visible
		if (dist > maxDrawDist)
			continue;

		// Too far away to split further (or max zoom is reached)
		if (dist > size || t.z==maxZoom)
		{
			neededTiles.Add(t);
		}
		// Close enough to split
		else
		{
			int zoom = t.z + 1;
			int x = t.x << 1;
			int y = t.y << 1;
			Vec3i c0 = Vec3i(x, y, zoom);
			Vec3i c1 = Vec3i(x+1, y, zoom);
			Vec3i c2 = Vec3i(x+1, y+1, zoom);
			Vec3i c3 = Vec3i(x, y+1, zoom);
			stack.Push(c0);
			stack.Push(c1);
			stack.Push(c2);
			stack.Push(c3);
		}
	}

	int abba = 10;
	
#if 0
	for (int ty = -range; ty <= range; ty++)
	{
		for (int tx = -range; tx <= range; tx++)
		{
			const Vec3i t(camtile.x + tx, camtile.y + ty, camtile.z);
			MyTile* tile = m_Tiles.Get(t);

			if (!tile)
				tilesToLoad.Add(t);

			if (tile)
				tile->m_Frame = frame;
		}
	}

	// Add tile(s) to queue to stream
	if (tilesToLoad.Num()>0 && !tileToStream)
	{
		if (tileLock.try_lock())
		{
			if (!tileToStream)
				tileToStream = new MyTile(tilesToLoad[0]);
			// Must be inside lock. Otherwise a thread might set tileToStream to NULL before we
			// add it to the hash table
			m_Tiles.Add(tileToStream->m_Tms, tileToStream);

			LOG("Added %d,%d for streaming in frame %d\n", tileToStream->m_Tms.x, tileToStream->m_Tms.y, frame);

			tileLock.unlock();
		}
	}

	// Any tiles done streaming?
	if (!doneList.IsEmpty())
	{
		StreamResult* r = NULL;
		if (doneLock.try_lock())
		{
			r = doneList.PopFront();
			doneLock.unlock();
		}

		if (r)
		{
			LOG("Received %d, %d in frame %d\n", r->tile->m_Tms.x, r->tile->m_Tms.y, frame);
			r->tile->m_Status = MyTile::eLoaded;
			//CVec3 pos = MercatorToGl(r->tile->m_Origo, 5.0f);
			//r->tile->SetPos(pos);
			//LOG("pos: <%.1f, %.1f, %.1f>\n", pos.x, pos.y, pos.z);

			for (int i = 0; i < r->geoms.Num(); i++)
			{
				CMesh* mesh = CreateMeshFromGeoms(r->geoms[i].name, &r->geoms[i], 1);
				Entity* e = new Entity(r->geoms[i].name);
				MeshComponent* meshcomp = e->CreateComponent<MeshComponent>();
				meshcomp->m_DrawableFlags.Set(Drawable::eLightMap);
				meshcomp->SetMesh(mesh, MeshComponent::eMeshDelete); // Resets dirty flag
				r->tile->AddChild(e);
			}

			r->tile->SetPos(MercatorToGl(r->tile->m_Origo, 5.0f)); // Must set position here, after children are added. Because children need their transform dirtied

			gScene.AddEntity(r->tile);


			r->tile = NULL;
			// Release geom memory
			poolLock.lock();
			geomPool.Free(r);
			poolLock.unlock();
		}
	}

	m_LastTile = camtile;
#endif
}
//-----------------------------------------------------------------------------
void TileManager::RenderDebug2d(CCamera* cam)
{
	int sw = gRenderer->GetScreenWidth();
	int sh = gRenderer->GetScreenHeight();
	Vec2 screenpos(sw / 2, sh / 2);

	float scale = 1.0f / 100.0f;

	const Vec2d camMercator2d = GlToMercator2d(cam->GetWorldPos());

	int minz = 200;
	int maxz = 0;
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