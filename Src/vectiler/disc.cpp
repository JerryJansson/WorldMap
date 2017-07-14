#include "Precompiled.h"
#include "geometry.h"
#include "tiledata.h"
#include "disc.h"
#include "..\..\..\..\Source\Middleware\lz4\lz4.h"
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
		//const Feature* feature = mesh->feature;

		f.WriteInt(mesh->layerType);

		/*int sort;
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
		}*/

		meshname = mesh->feature_name;
		int sort = mesh->feature_sortrank;
		EFeatureKind kind = mesh->feature_kind;

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


	/*f.Close();
	CStopWatch tt;
	int loadsize = 0;
	char* loaded = F_Load(fname, &loadsize);

	int bounds = LZ4_compressBound(loadsize);

	tt.Start();
	char* lz4 = new char[bounds];
	float t1 = tt.GetMs(true);
	int compressed = LZ4_compress_default(loaded, lz4, loadsize, bounds);
	float t2 = tt.GetMs();

	delete[] lz4;
	delete[] loaded;

	LOG("LZ4: Compressed %.1fkb -> %.1fkb (%.1f%%) in %.1f, %.1fms\n", loadsize/1024.0f, compressed/1024.0f, (compressed*100.0f) / loadsize, t1, t2);*/

	return true;
}
//-----------------------------------------------------------------------------
Crgba GetColor(const ELayerType l, const EFeatureKind k)
{
	static MapGeom default(Crgba(255, 0, 0), 0.1f, false);
	MapGeom* g = gGeomHash.GetValuePtr((l << 16 | k));
	return g ? g->color : default.color;
}
//-----------------------------------------------------------------------------
/*struct myV
{
	float d[6];
};
TArray<myV> vtmp(65536);
TArray <uint16>	itmp(40000);*/
//-----------------------------------------------------------------------------
bool LoadBin(const char* fname, TArray<StreamGeom*>& geoms)
{
	CStopWatch sw,sw1;

	CFile f;
	if (!f.Open(fname, FILE_READ))
		return false;

	float t1 = sw1.GetMs(true);

	int ver = f.ReadInt();

	if (ver != VER)
		return false;

	int nMeshes = f.ReadInt();

	int currLayer = -1;
	StreamGeom* bigGeom = NULL;
	int subLayer;

	float t2 = sw1.GetMs(true);

	float t3 = 0;
	float t4 = 0;
	float t5 = 0;
	float t6 = 0;
	float t7 = 0;
	for (int m = 0; m < nMeshes; m++)
	{
		CStopWatch sw2;
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

		t3 += sw2.GetMs(true);

		if (allocGeom)
		{
			bigGeom = new StreamGeom;
			//bigGeom->vertices.EnsureCapacity(65536);
			//bigGeom->indices.EnsureCapacity(65536);
			//vtmp.Clear();
			//itmp.Clear();
			bigGeom->layerType = layerType;
			bigGeom->layerSubIdx = subLayer;
			geoms.Add(bigGeom);
		}

		t4 += sw2.GetMs(true);

		const int voffset = bigGeom->vertices.Num();
		const int ioffset = bigGeom->indices.Num();
		bigGeom->vertices.AddEmpty(nv);
		bigGeom->indices.AddEmpty(ni);
		//bigGeom->vertices.SetNum(voffset + nv);
		//bigGeom->indices.SetNum(ioffset + ni);
		vtxMap* vtx = &bigGeom->vertices[voffset];
		uint16* idxs = &bigGeom->indices[ioffset];
		/*const int voffset = vtmp.Num();
		const int ioffset = itmp.Num();
		vtmp.SetNum(voffset + nv);
		itmp.SetNum(ioffset + ni);
		myV* vtx = &vtmp[voffset];
		uint16* idxs = &bigGeom->indices[ioffset];*/

		const Crgba col = GetColor(layerType, kind);
		if (col == gRGBA_Red)
		{
			int abba = 10;
		}

		t5 += sw2.GetMs(true);

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

		t6 += sw2.GetMs(true);

		f.Read(idxs, ni * sizeof(uint16));
		for (int i = 0; i < ni; i++)
			idxs[i] += voffset;

		t7 += sw2.GetMs(true);

		//LOG("Read: %s, type: %d, nv: %d, ni: %d\n", name, type, nv, ni);
	}

	LOG("Loaded %s in %.0fms. Merged %d -> %d\n", fname, sw.GetMs(), nMeshes, geoms.Num());
	//LOG("%.1fms, %.1fms, %.1fms, %.1fms, %.1fms, %.1fms, %.1fms <%d, %d>\n", t1, t2, t3, t4, t5, t6, t7, geoms[0]->vertices.Num(), geoms[0]->indices.Num());

	return true;
}