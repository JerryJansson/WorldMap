#include "Precompiled.h"

#if JJ_WORLDMAP == 2

#include "tilemanager2.h"
#include "projection.h"
#include "jerry.h"
#include "../../../../Source/Modules/GameObject/Camera.h"
#include "../../../../Source/Modules/Shared/SceneNew.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "disc.h"
#include "tiledata.h"
#include "vectiler.h"
//-----------------------------------------------------------------------------
const bool useSingleTile = false;		// Only load 1 tile. Good for debugging
CVar tile_QuadTree("tile_QuadTree", true);
CVar tile_ShowQuadTree("tile_ShowQuadTree", 0);
CVar tile_DiscCache("tile_DiscCache", true);
//-----------------------------------------------------------------------------
#define ZZZOOM 16							// Regular grid uses a fixed zoom
//const Vec2d longLatStart(18.080, 59.346);	// 36059, 19267 - Stockholm Stadion
const Vec2d longLatStart(-74.0130, 40.703);	// 19294, 24642 - Manhattan
//const Vec2d longLatStart(13.3974, 52.4974);	// 19294, 24642 - Berlin. Building with holes
//const Vec3i singleTile(35207, 44036, 16);	// Berlin. Building with holes
//const Vec3i singleTile(1204, 2554, 12);	// Contains a mesh > 65536 vertices
//const Vec3i singleTile(4820, 10224, 14);	// Contains a mesh == 65536 vertices
const Vec3i singleTile(19294, 40893, 16);
//const Vec3i singleTile(0,0,0);
//const Vec3i singleTile(2,1,2);
//const Vec3i singleTile(9647, 20446, 15);
//const Vec3i singleTile(19288, 40894, 16);
// http://tangrams.github.io/tangram/#52.49877546805043,13.397676944732667,17 // (complex building with holes)
//-----------------------------------------------------------------------------
// Threads & synchronization
//-----------------------------------------------------------------------------
const int numThreads = 1;
std::thread threads[numThreads];
std::mutex tileLock;
std::mutex poolLock;
std::mutex doneLock;
std::condition_variable signal_tileLock;
bool stop = false;
TileData* tileToStream = NULL;
//-----------------------------------------------------------------------------
#define NDATA 64
//static MemPoolDynamic<TileData> dataPool(NDATA);	// Enities
static TArray<TileData*> freeData;// (NDATA);
//static TileData* freeData = NULL;				// LRU Free list
static Hash<Vec3i, TileData*> dataHash;
//-----------------------------------------------------------------------------
static MemPoolDynamic<TileNode> nodePool(64);	// Nodes in quadtree
//static MemPoolDynamic<TileData> dataPool(64);	// Loaded data
//static TStackArray<TileData>	dataCache(64);	// Loaded data
TArray<TileData*> doneList;
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

		// Wait until main() signals
		//std::unique_lock<std::mutex> lk(tileLock);
		//signal_tileLock.wait(lk, [] {return ready; });

		LOG("Thread %d picked up <%d, %d, %d> in frame %d\n", threadIdx, tileToStream->m_Tms.x, tileToStream->m_Tms.y, tileToStream->m_Tms.z, Engine_GetFrameNumber());

		TileData* t = tileToStream;
		tileToStream = NULL;
		tileLock.unlock();

		//StreamResult* result = AllocResult(t);
		GetTile(t);

		doneLock.lock();
		doneList.Add(t);
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
	CreateKindHash();

	Vec3i tile;
	if (useSingleTile) // Debug
	{
		tile = singleTile;
	}
	else
	{
		Vec2d meters = LonLatToMeters(longLatStart);
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
		for (int i = 0; i < dataHash.Num(); i++)
		{
			TileData* data = dataHash[i].val;
			data->SetPos(MercatorToGl(data->m_Origo, 5.0f));
		}

		m_LastTile = camtile;
	}

	return camtile;
}
//-----------------------------------------------------------------------------
// Prio 1: In frustum
// Prio 2: Distance from camera
//-----------------------------------------------------------------------------
int ChooseTileToLoad(CCamera* cam, const TArray<TileNode*>& tiles)
{
	const CFrustum& f	= cam->GetFrustum();
	float mindist		= -1;
	bool haveVisible	= false;
	int minidx			= -1;

	for (int i = 0; i < tiles.Num(); i++)
	{
		const TileNode* node = tiles[i];
		bool visible = BoxIntersectsFrustum(f, node->m_NodeAabb);
		if (visible)
		{
			if (!haveVisible || minidx == -1 || node->m_Dist < mindist)
			{
				haveVisible = true;
				minidx = i;
				mindist = node->m_Dist;
			}
		}
		else if(!haveVisible)
		{
			if (minidx == -1 || node->m_Dist < mindist)
			{
				minidx = i;
				mindist = node->m_Dist;
			}
		}
	}

	return minidx;
}
//-----------------------------------------------------------------------------
// Alloc data
//-----------------------------------------------------------------------------
TileData* AllocTileData(TileNode* node)
{
	// See if data already exists in cache
	TileData* data = dataHash.Get(node->m_Tms);

	// Cached
	if (data)
	{
		data->SetEnabled(true);
	}
	// Not cached
	else
	{
		// No more free data. Must allocate some more
		/*if (freeData.Num() == 0)
		{ 
			data = dataPool.Alloc();	// Triggers pool growth
			int n = dataPool.GetFreeCount();
			freeData.EnsureCapacity(n);
			for (int i = 0; i < n; i++)
				freeData.Add(dataPool.Alloc());
			data = new TileData();
		}*/
		
		// We always want to keep a certain amount of old cached data
		static const int minimumOldDataCache = 16;

		int lru = -1;
		if (freeData.Num() > minimumOldDataCache)
		{
			uint32 minBump;
			for (int i = 0; i < freeData.Num(); i++)
			{
				TileData* d = freeData[i];
				// Don't touch tiles that are loading
				if (d->Status() == TileData::eLoading)
					continue;

				if (lru == -1 || d->m_FrameBump < minBump)
				{
					lru = i;
					minBump = d->m_FrameBump;
				}
			}
		}
		
		// Delete previously cached tile
		if (lru >= 0)
		{
			data = freeData[lru];
			freeData.Remove(lru);

			// Are we stealing cached data from another tile?
			assert(dataHash.Get(data->m_Tms));
			dataHash.Remove(data->m_Tms);	// Remove from cache
			delete(data);					// Remove from scene (only in scene if status==eLoaded)
		}
		
		data = new TileData();
		data->Init(node->m_Tms);
		dataHash.Add(data->m_Tms, data);	// Add to cache
	}

	data->m_FrameBump = Engine_GetFrameNumber();
	data->m_Node = node;
	node->m_Data = data;

	return data;
}
//-----------------------------------------------------------------------------
// When we release the tile data, we only break the coupling between the
// node & the data. Since the data can be loading we keep it alive
// until it's done loading and then put it back in the pool
//-----------------------------------------------------------------------------
void ReleaseTileData(TileNode* node)
{
	TileData* data = node->m_Data;
	data->SetEnabled(false);		// Disable it so it won't update & render in scene
	freeData.Add(data);				// Add to free list
	data->m_Node = NULL;			// Release coupling between node & data
	node->m_Data = NULL;
}
//-----------------------------------------------------------------------------
static inline TileNode* AllocNode(const Vec3i& tms, TileNode* parent)
{
	TileNode* n = nodePool.Alloc();
	n->m_Tms = tms;
	n->m_Parent = parent;
	n->m_Data = NULL;
	n->m_Child[0] = NULL;
	n->m_Child[1] = NULL;
	n->m_Child[2] = NULL;
	n->m_Child[3] = NULL;
	return n;
}
//-----------------------------------------------------------------------------
// Return node (and children) to memory pool
// Note. Children may or may not be loaded. They might even currently be loading.
// This does not matter since we just release the coupling between the
// node and the data. If a tile is currently loading and finishes without having
// a coupling to a node, nothing happens. The data is just kept in the cache
// for a while.
//-----------------------------------------------------------------------------
static inline void ReleaseNode(TileNode** nodeptr)
{
	TileNode* node = *nodeptr;
	if (node->m_Data)
		ReleaseTileData(node);
	
	if (node->m_Child[0])
	{
		ReleaseNode(&node->m_Child[0]);
		ReleaseNode(&node->m_Child[1]);
		ReleaseNode(&node->m_Child[2]);
		ReleaseNode(&node->m_Child[3]);
	}

	*nodeptr = NULL;
}

//-----------------------------------------------------------------------------
// Check if childrens loaded data covers up my entire space
//-----------------------------------------------------------------------------
bool CoveredByChildren(const TileNode* node)
{
	if (!node->m_Child[0])
		return false;

	for (int i = 0; i < 4; i++)
	{
		if (!node->m_Child[i]->IsLoaded() && !CoveredByChildren(node->m_Child[i]))
			return false;
	}

	return true;
}
//-----------------------------------------------------------------------------
TArray<TileNode*> neededTiles;
//-----------------------------------------------------------------------------
void TileManager::UpdateQTree(CCamera* cam)
{
	const uint32 frame = Engine_GetFrameNumber();
	const Vec3 camGlPos = cam->GetWorldPos();
	const Vec3d camMercator3d = GlToMercator3d(camGlPos);
	const float maxDrawDist = 12 * 1000; // 12km
	const int maxZoom = 17;
	int n = 0;

	if (!m_RootNode)
		m_RootNode = AllocNode(Vec3i(0, 0, 0), NULL);
	
	TStack<TileNode*, 48> stack(m_RootNode);
	while (stack.Count())
	{
		n++;
		TileNode* node = stack.Pop();
		const Vec3i& tms = node->m_Tms;
		const Vec2d tileMin = TileMin(tms);
		const Vec2d tileMax = TileMax(tms);
		const float size = (tileMax.x - tileMin.x);
		Caabb& aabb = node->m_NodeAabb;
		aabb = Caabb(MercatorToGl(tileMin, 0.0f), MercatorToGl(tileMax, 1000.0f));
		Swap(aabb.m_Min.z, aabb.m_Max.z);
		
		node->m_Dist = DistancePointAabb(camGlPos, aabb);

		// To far away to be visible
		//if (dist > maxDrawDist)
		//	continue;

		// Too far away to split (or max zoom is reached)
		// This is a leaf (should be loaded and rendered)
		// If we have children, they are now too high detail, release them as soon as parent is loaded and can replace them visually
		if (node->m_Dist > size / 2.0f || tms.z >= maxZoom)
		{
			neededTiles.Add(node);	// Add to list of possible streamers

			TileData* data = node->m_Data;
			
			// We have no data, but need it
			if (!data)
				data = AllocTileData(node);

			data->m_FrameBump = frame;	// Mark data as freshly used

			// As soon as this node is loaded, we can release the children, if any
			if (node->m_Child[0] && data->IsLoaded())
			{
				ReleaseNode(&node->m_Child[0]);
				ReleaseNode(&node->m_Child[1]);
				ReleaseNode(&node->m_Child[2]);
				ReleaseNode(&node->m_Child[3]);
			}
		}
		// Close enough to split
		else
		{
			if (!node->m_Child[0])
			{
				const int zoom = tms.z + 1;
				const int x = tms.x << 1;
				const int y = tms.y << 1;
				node->m_Child[0] = AllocNode(Vec3i(x, y, zoom), node);
				node->m_Child[1] = AllocNode(Vec3i(x + 1, y, zoom), node);
				node->m_Child[2] = AllocNode(Vec3i(x + 1, y + 1, zoom), node);
				node->m_Child[3] = AllocNode(Vec3i(x, y + 1, zoom), node);
			}

			stack.Push(node->m_Child[0]);
			stack.Push(node->m_Child[1]);
			stack.Push(node->m_Child[2]);
			stack.Push(node->m_Child[3]);

			// No longer need this data if children covers my entire visual area
			if (node->m_Data)
			{
				if (CoveredByChildren(node))
					ReleaseTileData(node);
			}
		}
	}
}
//-----------------------------------------------------------------------------
void TileManager::UpdateSingleTile(const Vec3i& tms)
{
	if (!m_RootNode)
	{
		m_RootNode = AllocNode(tms, NULL);
		AllocTileData(m_RootNode);
	}
	neededTiles.Add(m_RootNode);
}
//-----------------------------------------------------------------------------
void TileManager::Update(CCamera* cam)
{
	if (!m_Initialized)
		Initialize(cam);

	ShiftOrigo(cam);

	const Vec3 cpos = cam->GetWorldPos();
	const Vec2d meters = GlToMercator2d(cpos);
	const Vec2d ll = MetersToLongLat(meters);
	DbgMsg("cam: gl-<%.1f, %.1f, %.1f>, long/lat <%.5f, %.5f>", cpos.x, cpos.y, cpos.z, ll.x, ll.y);

	// Determine needed tiles
	neededTiles.Clear();
	if (useSingleTile)	UpdateSingleTile(singleTile);
	else				UpdateQTree(cam);

	// Determine missing tiles
	static TArray<TileNode*> missingTiles(256);
	missingTiles.Clear();
	for (int i = 0; i < neededTiles.Num(); i++)
	{
		TileNode* node = neededTiles[i];
		if(node->m_Data && node->m_Data->Status() == TileData::eNotLoaded)
			missingTiles.Add(node);
	}

	DbgMsg("Tiles <Needed: %d, Missing: %d, Cached: %d>", neededTiles.Num(), missingTiles.Num(), dataHash.Num());

	// Add tile to queue to stream
	if (missingTiles.Num()>0 && !tileToStream)
	{
		const int loadIdx = ChooseTileToLoad(cam, missingTiles);

		if (tileLock.try_lock())	// No need to verify !tileToStream inside lock. Main thread is the only thread that can set tileToStream!=NULL
		{
			TileData* t = missingTiles[loadIdx]->m_Data;
			t->SetStatus(TileData::eLoading);
			tileToStream = t;
			tileLock.unlock();
			LOG("Added <%d, %d, %d> for streaming in frame %d\n", t->m_Tms.x, t->m_Tms.y, t->m_Tms.z, Engine_GetFrameNumber());
		}

		//if (newtile)
		//	m_Tiles.Add(newtile->m_Tms, newtile);

		//float t = sw.GetMs(true);
		//LOG("Choose tile, lock, add to hash: %.2fms\n", t);
	}

	// Any tiles done streaming?
	ReceiveLoadedTile();
}
//-----------------------------------------------------------------------------
CMesh* CreateMeshFromStreamedGeom(const char* name, const StreamGeom* g)
{
	const VertexFmtID vfid = vtxMap::fmt;
	const int vertexSize = gRenderer->GetVertexFormat(vfid)->Stride();
	const int nv = g->vertices.Num();
	const int ni = g->indices.Num();

	// Create mesh
	CMesh* mesh = new CMesh(name, 1);

	// Create 1 common index buffer
	CIndexBuffer* ib = gRenderer->CreateIndexBuffer(IDX_SHORT, ni, BUF_STATIC, g->indices.Ptr());
	CVertexBuffer* vb = gRenderer->CreateVertexBuffer(vertexSize, nv, BUF_STATIC, g->vertices.Ptr());

	SubMesh& range = mesh->m_SubMeshes[0];
	range.glgeomIdx = 0;
	range.firstIdx = 0;
	range.numIdx = ni;
	range.firstVtx = 0;
	range.numVtx = nv;
	range.materialName = "buildings";// g->materialName.Str();
	range.geometricCenter = g->aabb.GetCenter();

	mesh->m_MeshAabb = g->aabb;
	mesh->m_GlGeoms.SetNum(1);
	mesh->m_GlGeoms[0].SetBuffers(vfid, vb, ib, GlGeom::eOwnsAll);

	//if (o->createCollision)
	//	mesh->m_Collision = CreateCollisionModelFromGeoms(geoms, nGeoms);

	return mesh;
}
//-----------------------------------------------------------------------------
bool TileManager::ReceiveLoadedTile()
{
	CStopWatch sw;

	// No tiles waiting to be received
	if (doneList.Num() == 0)
		return false;

	TileData* t = NULL;
	if (doneLock.try_lock())
	{
		if (doneList.Num())
		{
			t = doneList[0];
			doneList.RemoveOrdered(0);
		}
		doneLock.unlock();
	}

	if (!t)
		return false;

	float t_1 = sw.GetMs();

	const int n = t->geoms.Num();
	for (int i = 0; i < n; i++)
	{
		CStrL name = Str_Printf("%s_%d", layerNames[t->geoms[i]->layerType], t->geoms[i]->layerSubIdx);
		CMesh* mesh = CreateMeshFromStreamedGeom(name, t->geoms[i]);
		Entity* e = new Entity(name);
		MeshComponent* meshcomp = e->CreateComponent<MeshComponent>();
		meshcomp->m_DrawableFlags.Set(Drawable::eLightMap);
		meshcomp->SetMesh(mesh, MeshComponent::eMeshDelete); // Resets dirty flag
		t->AddChild(e);
	}

	float t_createMeshes = sw.GetMs() - t_1;

	t->SetPos(MercatorToGl(t->m_Origo, 5.0f)); // Must set position here, after children are added. Because children need their transform dirtied
	gScene.AddEntity(t);
	t->SetStatus(TileData::eLoaded);

	// Release geom memory
	for (int i = 0; i < t->geoms.Num(); i++)
		delete t->geoms[i];
	t->geoms.Clear();

	LOG("%.1fms. Received <%d, %d, %d> in frame %d. Created %d meshes in %.1fms\n", sw.GetMs(), t->m_Tms.x, t->m_Tms.y, t->m_Tms.z, Engine_GetFrameNumber(), n, t_createMeshes);

	return true;
}
//-----------------------------------------------------------------------------
void TileManager::RenderDebug2d(CCamera* cam)
{
	if (!tile_ShowQuadTree)
		return;

	int sw = gRenderer->GetScreenWidth();
	int sh = gRenderer->GetScreenHeight();
	Vec2 screenpos(sw / 2, sh / 2);

	const Vec2d camMercator2d = GlToMercator2d(cam->GetWorldPos());

	int minz = 200;
	int maxz = 0;

	
	int n = 0;
	for (int i = 0; i < neededTiles.Num(); i++)
	{
		const TileNode* t = neededTiles[i];
		minz = Min(minz, t->m_Tms.z);
		maxz = Max(maxz, t->m_Tms.z);

		Vec2d pmind = (TileMin(t->m_Tms) - camMercator2d) / 35.0;
		Vec2d pmaxd = (TileMax(t->m_Tms) - camMercator2d) / 35.0;
		Vec2 pmin = Vec2(pmind.x, pmind.y);
		Vec2 pmax = Vec2(pmaxd.x, pmaxd.y);
		pmin += screenpos;
		pmax += screenpos;
		DrawWireRect(pmin, pmax, gRGBA_Red);
		n++;
	}

	FlushLines_2d();

	DrawWireCircle(screenpos, 3.0f, gRGBA_Black);
	DbgMsg("zoom: %d-%d", minz, maxz);
	DbgMsg("Drew %d quads", n);
}


TileManager gTileManager;

#endif