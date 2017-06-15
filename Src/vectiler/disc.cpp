#include "Precompiled.h"
#include "geometry.h"
#include "tiledata.h"
#include "disc.h"
//-----------------------------------------------------------------------------
#define VER 2
#define MAX_SORT_RANK 500 // https://mapzen.com/documentation/vector-tiles/layers/
//-----------------------------------------------------------------------------
bool SaveBin(const char* fname, const std::vector<PolygonMesh*>& meshes)
{
	CStopWatch sw;

	CFile f;
	if (!f.Open(fname, FILE_WRITE))
		return false;

	TArray<uint16> idx16bit;
	const int nMeshes = meshes.size();
	f.WriteInt(VER);
	f.WriteInt(nMeshes);

	int statNumVertices = 0;
	int statNumTriangles = 0;

	CStrL meshname;
	for (int i = 0; i < nMeshes; i++)
	{
		const PolygonMesh* mesh = meshes[i];
		const Feature* feature = mesh->feature;

		f.WriteInt(mesh->layerType);

		int sort;
		int kind;
		if (feature)
		{
			meshname = feature->name;
			sort = feature->sort_rank;
			kind = feature->kind;
		}
		else
		{
			meshname = "";
			sort = 0;
			kind = eKind_unknown;
		}

		f.WriteInt(kind);
		f.WriteInt(sort);
		f.WriteInt(meshname.Len());
		f.Write(meshname.Str(), meshname.Len() * sizeof(char));
		f.WriteInt(mesh->vertices.size());
		f.WriteInt(mesh->indices.size());

		for (auto v : mesh->vertices)
		{
			// Flip
			float p[6];
			p[0] = v.position.x;
			p[1] = v.position.z;
			p[2] = -v.position.y;
			p[3] = v.normal.x;
			p[4] = v.normal.z;
			p[5] = -v.normal.y;
			f.Write(p, sizeof(float) * 6);
		}

		const int ni = mesh->indices.size();
		idx16bit.SetNum(ni);
		for (int i = 0; i < ni; i++)
			idx16bit[i] = mesh->indices[i];

		f.Write(idx16bit.Ptr(), ni * sizeof(uint16));
		//f.Write(mesh->indices.data(), mesh->indices.size() * sizeof(int));
		statNumVertices += mesh->vertices.size();
		statNumTriangles += mesh->indices.size() / 3;
	}

	LOG("%.1fms. Saved %s. Meshes: %d, Tris: %d, Vtx: %d\n", sw.GetMs(), fname, nMeshes, statNumTriangles, statNumVertices);

	return true;
}
//-----------------------------------------------------------------------------
bool LoadBin(const char* fname, TArray<StreamGeom*>& geoms)
{
	CStopWatch sw;

	CFile f;
	if (!f.Open(fname, FILE_READ))
		return false;

	int ver = f.ReadInt();

	if (ver != VER)
		return false;

	int nMeshes = f.ReadInt();

	int currLayer = -1;
	StreamGeom* bigGeom = NULL;
	int subLayer;

	for (int m = 0; m < nMeshes; m++)
	{
		ELayerType layerType = (ELayerType)f.ReadInt();
		EFeatureKind kind = (EFeatureKind)f.ReadInt();
		const int sort = f.ReadInt();
		const float fSort = (float)sort/MAX_SORT_RANK;
		const int nlen = f.ReadInt();
		if (nlen >= 128)
		{
			assert(0);
			gLogger.Warning("Name to long");
			return false;
		}

		char name[128];
		f.Read(name, nlen * sizeof(char));
		name[nlen] = '\0';
		const int nv = f.ReadInt();
		const int ni = f.ReadInt();

		bool allocGeom = false;
		if (layerType != currLayer)
		{
			allocGeom = true;
			currLayer = layerType;
			subLayer = 0;
		}
		else if (bigGeom->vertices.Num() + nv > 65536)
		{
			allocGeom = true;
			subLayer++;
		}

		if (allocGeom)
		{
			bigGeom = new StreamGeom;
			bigGeom->vertices.EnsureCapacity(65536);
			bigGeom->indices.EnsureCapacity(65536);
			bigGeom->layerType = layerType;
			bigGeom->layerSubIdx = subLayer;
			geoms.Add(bigGeom);
		}

		const int voffset = bigGeom->vertices.Num();
		const int ioffset = bigGeom->indices.Num();
		bigGeom->vertices.SetNum(voffset + nv);
		bigGeom->indices.SetNum(ioffset + ni);
		vtxMap* vtx = &bigGeom->vertices[voffset];
		uint16* idxs = &bigGeom->indices[ioffset];

		const Crgba col = kindColors[kind];

		for (int i = 0; i < nv; i++)
		{
			float v[6];
			f.Read(v, sizeof(v[0]) * 6);
			vtxMap& p = vtx[i];
			p.pos = Vec4(v[0], v[1], v[2], fSort);
			p.nrm = &v[3];
			p.col = col;
			bigGeom->aabb.AddPoint(v);
		}

		f.Read(idxs, ni * sizeof(uint16));
		for (int i = 0; i < ni; i++)
			idxs[i] += voffset;

		//LOG("Read: %s, type: %d, nv: %d, ni: %d\n", name, type, nv, ni);
	}

	LOG("%.0fms. Loaded %s. Merged %d -> %d\n", sw.GetMs(), fname, nMeshes, geoms.Num());

	return true;
}

//-----------------------------------------------------------------------------
#if 0
struct MergeInfo
{
	int bigMeshIdx;
	int voffset;
	int ioffset;
};
//-----------------------------------------------------------------------------
// Adds a mesh to a big mesh. Returns voffset & ioffs where the small mesh starts
//-----------------------------------------------------------------------------
bool AddMeshToMesh(const PolygonMesh* src, PolygonMesh* dst, int& voffset, int& ioffset)
{
	voffset = dst->vertices.size();
	ioffset = dst->indices.size();

	if (voffset + src->vertices.size() > 65536)
		return false;

	dst->vertices.insert(dst->vertices.end(), src->vertices.begin(), src->vertices.end());

	dst->indices.reserve(dst->indices.size() + src->indices.size());
	for (size_t i = 0; i < src->indices.size(); i++)
		dst->indices.push_back(voffset + src->indices[i]);

	return true;
}
//-----------------------------------------------------------------------------
// Merge as many small meshes together as possible (<65536 vertices)
//-----------------------------------------------------------------------------
MergeInfo* MergeMeshes(const std::vector<PolygonMesh*>& meshes, std::vector<PolygonMesh*>& merged)
{
	const int nMeshes = meshes.size();
	if (nMeshes == 0)
		return;

	MergeInfo* mergeInfos = new MergeInfo[nMeshes];

	ELayerType layer = (ELayerType)-1;
	PolygonMesh* bigMesh = NULL;

	for (int i = 0; i < nMeshes; i++)
	{
		const PolygonMesh* mesh = meshes[i];
		int voffs, ioffs;

		bool allocNew = false;
		if (mesh->layerType != layer)
		{
			layer = mesh->layerType;
			allocNew = true;
		}
		else if (!AddMeshToMesh(mesh, bigMesh, voffs, ioffs))				// Try to add current mesh to our bigMesh
		{
			allocNew = true;
		}

		if (allocNew)
		{
			bigMesh = new PolygonMesh(layer);
			merged.push_back(bigMesh);
			AddMeshToMesh(mesh, bigMesh, voffs, ioffs);
		}

		mergeInfos[i].bigMeshIdx = merged.size() - 1;
		mergeInfos[i].voffset = voffs;
		mergeInfos[i].ioffset = ioffs;
	}

	return mergeInfos;
}
//-----------------------------------------------------------------------------
/*
	2	Ver
	462 nMeshes
	12	nMerged
	----------------	// Big meshes
	2			layer
	65536		vertex count
	33400		index count
	v0, v1, ... vertices (PN)
	i0, i1, ... indices (uint16)
	----------------
	...
	---------------- // Meshes
	0			big idx


*/
//-----------------------------------------------------------------------------
bool SaveBinMerged(const char* fname, std::vector<PolygonMesh*> meshes)
{
	CStopWatch sw;

	CFile f;
	if (!f.Open(fname, FILE_WRITE))
		return false;

	// Merge meshes
	std::vector<PolygonMesh*> merged;
	MergeInfo* mergeInfos = MergeMeshes(meshes, merged);

	TArray<uint16> idx16bit;

	const int nMeshes = meshes.size();
	const int nMerged = merged.size();
	
	f.WriteInt(VER);
	f.WriteInt(nMeshes);
	f.WriteInt(nMerged);

	for (int i = 0; i < nMerged; i++)
	{
		PolygonMesh* mesh = merged[i];
		f.WriteInt(mesh->layerType);
		f.WriteInt(mesh->vertices.size());
		f.WriteInt(mesh->indices.size());

		for (auto v : mesh->vertices)
		{
			// Flip
			float p[6];
			p[0] = v.position.x;
			p[1] = v.position.z;
			p[2] = -v.position.y;
			p[3] = v.normal.x;
			p[4] = v.normal.z;
			p[5] = -v.normal.y;
			f.Write(p, sizeof(float) * 6);
		}

		const int ni = mesh->indices.size();
		idx16bit.SetNum(ni);
		for (int i = 0; i < ni; i++)
			idx16bit[i] = mesh->indices[i];

		f.Write(idx16bit.Ptr(), ni * sizeof(uint16));
	}

	int statNumVertices = 0;
	int statNumTriangles = 0;

	CStrL meshname;
	for (int i = 0; i < nMeshes; i++)
	{
		const PolygonMesh* mesh = meshes[i];
		const Feature* feature = mesh->feature;
		MergeInfo* mi = &mergeInfos[i];

		f.WriteInt(mi->bigMeshIdx);

		int sort;
		int kind;
		if (feature)
		{
			meshname = feature->name;
			sort = feature->sort_rank;
			kind = feature->kind;
		}
		else
		{
			meshname = "";
			sort = 0;
			kind = eKindUnknown;
		}

		f.WriteInt(kind);
		f.WriteInt(sort);
		f.WriteInt(meshname.Len());
		f.Write(meshname.Str(), meshname.Len() * sizeof(char));
		f.WriteInt(mesh->vertices.size());
		f.WriteInt(mesh->indices.size());

		for (auto v : mesh->vertices)
		{
			// Flip
			float p[6];
			p[0] = v.position.x;
			p[1] = v.position.z;
			p[2] = -v.position.y;
			p[3] = v.normal.x;
			p[4] = v.normal.z;
			p[5] = -v.normal.y;
			f.Write(p, sizeof(float) * 6);
		}

		f.Write(mesh->indices.data(), mesh->indices.size() * sizeof(int));
		statNumVertices += mesh->vertices.size();
		statNumTriangles += mesh->indices.size() / 3;
	}

	LOG("Saved %s in %.1fms. Meshes: %d, Tris: %d, Vtx: %d\n", fname, sw.GetMs(), nMeshes, statNumTriangles, statNumVertices);

	return true;
}
#endif