#include "Precompiled.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "geometry.h"
#include "../../../Source/Modules/Shared/SceneNew.h"
//-----------------------------------------------------------------------------
const char* layerNames[eNumLayerTypes] =
{
	"terrain",
	"buildings",
	"roads",
	"water",
	"other"
};
//-----------------------------------------------------------------------------
void computeNormals(PolygonMesh& mesh)
{
	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		int i1 = mesh.indices[i + 0];
		int i2 = mesh.indices[i + 1];
		int i3 = mesh.indices[i + 2];

		const v3& p1 = mesh.vertices[i1].position;
		const v3& p2 = mesh.vertices[i2].position;
		const v3& p3 = mesh.vertices[i3].position;

		v3 d = myNormalize(myCross(p2 - p1, p3 - p1));

		mesh.vertices[i1].normal += d;
		mesh.vertices[i2].normal += d;
		mesh.vertices[i3].normal += d;
	}

	for (auto& v : mesh.vertices) {
		v.normal = myNormalize(v.normal);
	}
}
//-----------------------------------------------------------------------------
bool AddMeshToMesh(const PolygonMesh* src, PolygonMesh* dst)
{
	const int voffset = dst->vertices.size();
	int nv = src->vertices.size();

	if (voffset + nv > 65536)
		return false;

	dst->vertices.insert(dst->vertices.end(), src->vertices.begin(), src->vertices.end());
	
	for (size_t i = 0; i < src->indices.size(); i++)
	{
		dst->indices.push_back(voffset + src->indices[i]);
	}

	return true;
}
//-----------------------------------------------------------------------------
void addFaces(std::ostream& file, const PolygonMesh& mesh, size_t indexOffset, bool normals)
{
	for (size_t i = 0; i < mesh.indices.size(); i += 3)
	{
		file << "f " << mesh.indices[i] + indexOffset + 1 << (normals ? "//" + std::to_string(mesh.indices[i] + indexOffset + 1) : "");
		file << " ";
		file << mesh.indices[i + 1] + indexOffset + 1 << (normals ? "//" + std::to_string(mesh.indices[i + 1] + indexOffset + 1) : "");
		file << " ";
		file << mesh.indices[i + 2] + indexOffset + 1 << (normals ? "//" + std::to_string(mesh.indices[i + 2] + indexOffset + 1) : "");
		file << "\n";
	}
}
//-----------------------------------------------------------------------------
void addNormals(std::ostream& file, const PolygonMesh& mesh)
{
	// JJ Flip
	for (auto v : mesh.vertices)
		file << "vn " << v.normal.x << " " << v.normal.z << " " << -v.normal.y << "\n";
	//for (auto vertex : mesh.vertices)
	//	file << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
}
//-----------------------------------------------------------------------------
void addPositions(std::ostream& file, const PolygonMesh& mesh, float offsetx, float offsety)
{
	for (auto v : mesh.vertices)
	{
		// JJ - flip
		file << "v " << v.position.x + offsetx + mesh.offset.x << " " << v.position.z << " " << -v.position.y + offsety + mesh.offset.y	<< "\n";
		//file << "v " << v.position.x + offsetx + mesh.offset.x << " " << v.position.y + offsety + mesh.offset.y << " " << v.position.z << "\n";
	}
}
/*-----------------------------------------------------------------------------
* Save an obj file for the set of meshes
* - outputOBJ: the output filename of the wavefront object file
* - splitMeshes: will enable exporting meshes as single objects within
*   the wavefront file
* - offsetx/y: are global offset, additional to the inner mesh offset
* - append: option will append meshes to an existing obj file
*   (filename should be the same)
-----------------------------------------------------------------------------*/
bool saveOBJ(const char* outputOBJ,
	bool splitMeshes,
	//std::vector<std::unique_ptr<PolygonMesh>>& meshes,
	std::vector<PolygonMesh*>& meshes,
	float offsetx,
	float offsety,
	bool append,
	bool normals)
{
	size_t maxindex = 0;

	/// Find max index from previously existing wavefront vertices
	{
		std::ifstream filein(outputOBJ, std::ios::in);
		std::string token;

		if (filein.good() && append)
		{
			// TODO: optimize this
			while (!filein.eof())
			{
				filein >> token;
				if (token == "f") {
					std::string faceLine;
					getline(filein, faceLine);

					for (unsigned int i = 0; i < faceLine.length(); ++i)
					{
						if (faceLine[i] == '/')
							faceLine[i] = ' ';
					}

					std::stringstream ss(faceLine);
					std::string faceToken;

					for (int i = 0; i < 6; ++i)
					{
						ss >> faceToken;
						if (faceToken.find_first_not_of("\t\n ") != std::string::npos)
						{
							size_t index = atoi(faceToken.c_str());
							maxindex = index > maxindex ? index : maxindex;
						}
					}
				}
			}

			filein.close();
		}
	}

	/// Save obj file
	{
		std::ofstream file(outputOBJ);

		if (append)
			file.seekp(std::ios_base::end);

		if (file.is_open())
		{
			size_t nVertex = 0;
			size_t nTriangles = 0;

			file << "# exported with vectiler: https://github.com/karimnaaji/vectiler" << "\n";
			file << "\n";

			int indexOffset = maxindex;

			if (splitMeshes)
			{
				int meshCnt = 0;

				for (const auto& mesh : meshes)
				{
					if (mesh->vertices.size() == 0) { continue; }

					file << "o mesh" << meshCnt++ << "\n";

					addPositions(file, *mesh, offsetx, offsety);
					nVertex += mesh->vertices.size();

					if (normals)
						addNormals(file, *mesh);

					addFaces(file, *mesh, indexOffset, normals);
					nTriangles += mesh->indices.size() / 3;

					file << "\n";

					indexOffset += mesh->vertices.size();
				}
			}
			else
			{
				file << "o " << outputOBJ << "\n";

				for (const auto& mesh : meshes)
				{
					if (mesh->vertices.size() == 0) continue;
					addPositions(file, *mesh, offsetx, offsety);
					nVertex += mesh->vertices.size();
				}

				if (normals)
				{
					for (const auto& mesh : meshes)
					{
						if (mesh->vertices.size() == 0) continue;
						addNormals(file, *mesh);
					}
				}

				for (const auto& mesh : meshes)
				{
					if (mesh->vertices.size() == 0) continue;
					addFaces(file, *mesh, indexOffset, normals);
					indexOffset += mesh->vertices.size();
					nTriangles += mesh->indices.size() / 3;
				}
			}

			file.close();

			// Print infos
			{
				LOG("Saved obj file: %s\n", outputOBJ);
				LOG("Triangles: %ld\n", nTriangles);
				LOG("Vertices: %ld\n", nVertex);

				std::ifstream in(outputOBJ, std::ifstream::ate | std::ifstream::binary);
				if (in.is_open())
				{
					int size = (int)in.tellg();
					LOG("File size: %fmb\n", float(size) / (1024 * 1024));
					in.close();
				}
			}

			return true;
		}
		else {
			LOG("Can't open file %s\n", outputOBJ);
		}
	}

	return false;
}


//-----------------------------------------------------------------------------
void WriteMesh(FILE* f, const PolygonMesh* mesh, const char* layerName, const int meshIdx, const int idxOffset)
{
	fprintf(f, "o %s_%d\n", layerName, meshIdx);
	for (auto v : mesh->vertices)
	{
		// JJ - flip
		fprintf(f, "v %f %f %f\n", v.position.x, v.position.z, -v.position.y);
		//file << "v " << v.position.x + offsetx + mesh.offset.x << " " << v.position.y + offsety + mesh.offset.y << " " << v.position.z << "\n";
	}
	for (auto v : mesh->vertices)
	{
		fprintf(f, "vn %f %f %f\n", v.normal.x, v.normal.z, -v.normal.y);
	}

	for (size_t i = 0; i < mesh->indices.size(); i += 3)
	{
		int idx0 = mesh->indices[i+0] + idxOffset + 1;
		int idx1 = mesh->indices[i+1] + idxOffset + 1;
		int idx2 = mesh->indices[i+2] + idxOffset + 1;
		fprintf(f, "f %d//%d %d//%d %d//%d\n", idx0, idx0, idx1, idx1, idx2, idx2);
	}

	//indexOffset += mesh->vertices.size();
}
//-----------------------------------------------------------------------------
bool SaveOBJ2(const char* fname, std::vector<PolygonMesh*> meshArr[eNumLayerTypes])
{
	FILE* f = fopen(fname, "wt");
	if (!f)
		return false;

	fprintf(f, "# exported by Jerry's WorldMap\n\n");

	int statNumObjects = 0;
	int statNumVertices = 0;
	int statNumTriangles = 0;

	int idxOffset = 0;
	for (int i = 0; i < eNumLayerTypes; i++)
	{
		for (size_t k = 0; k < meshArr[i].size(); k++)
		{
			WriteMesh(f, meshArr[i][k], layerNames[i], k, idxOffset);
			idxOffset += meshArr[i][k]->vertices.size();

			statNumObjects++;
			statNumVertices += meshArr[i][k]->vertices.size();
			statNumTriangles += meshArr[i][k]->indices.size()/3;
		}
	}

	fclose(f);

	LOG("Saved obj file: %s\n", fname);
	LOG("Objects: %ld\n", statNumObjects);
	LOG("Triangles: %ld\n", statNumTriangles);
	LOG("Vertices: %ld\n", statNumVertices);

	return true;
}


//-----------------------------------------------------------------------------
void WriteMeshBin(CFile& f, const PolygonMesh* mesh, const char* layerName, const int meshIdx)
{
	CStrL meshname = Str_Printf("%s_%d", layerName, meshIdx);
	f.WriteInt(meshname.Len());
	f.Write(meshname.Str(), meshname.Len() * sizeof(char));
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

	f.Write(mesh->indices.data(), mesh->indices.size() * sizeof(int));

	/*for (size_t i = 0; i < mesh->indices.size(); i++)
	{
		uint16 u16 = mesh->indices[i];
		f.Write(&u16, sizeof(u16));
	}*/
}
#define VER 1
//-----------------------------------------------------------------------------
bool SaveBin(const char* fname, std::vector<PolygonMesh*> meshArr[eNumLayerTypes])
{
	CFile f;
	if (!f.Open(fname, FILE_WRITE))
		return false;

	int nMeshes = 0;
	for (int l = 0; l < eNumLayerTypes; l++)
		nMeshes += meshArr[l].size();
	
	f.WriteInt(VER);
	f.WriteInt(nMeshes);

	int statNumVertices = 0;
	int statNumTriangles = 0;

	for (int l = 0; l < eNumLayerTypes; l++)
	{
		for (size_t k = 0; k < meshArr[l].size(); k++)
		{
			WriteMeshBin(f, meshArr[l][k], layerNames[l], k);
			statNumVertices += meshArr[l][k]->vertices.size();
			statNumTriangles += meshArr[l][k]->indices.size() / 3;
		}
	}

	LOG("Saved obj file: %s\n", fname);
	LOG("Objects: %ld\n", nMeshes);
	LOG("Triangles: %ld\n", statNumTriangles);
	LOG("Vertices: %ld\n", statNumVertices);

	return true;
}
//-----------------------------------------------------------------------------
bool LoadBin(const char* fname)
{
	CFile f;
	if (!f.Open(fname, FILE_READ))
		return false;

	int ver = f.ReadInt();

	if (ver != VER)
	{
		gLogger.TellUser(LOG_ERROR, "Wrong version");
		return false;
	}

	int nMeshes = f.ReadInt();

	GGeom geom;
	for(int m=0; m<nMeshes; m++)
	{
		int nlen = f.ReadInt();
		if (nlen >= 64)
		{
			gLogger.Warning("Name to long");
			return false;
		}

		char name[64];
		int type, nv, ni;

		f.Read(name, nlen * sizeof(char));
		name[nlen] = '\0';
		f.Read(&type, sizeof(type));
		f.Read(&nv, sizeof(nv));
		f.Read(&ni, sizeof(ni));

		geom.Prepare(nv, ni);
		GVertex* vtx = geom.VertexPtr();
		uint32* idxs = geom.IndexPtr();

		for (int i = 0; i < nv; i++)
		{
			float v[6];
			f.Read(v, sizeof(v[0]) * 6);
			vtx[i].pos = &v[0];
			vtx[i].nrm = &v[3];
		}

		f.Read(idxs, ni * sizeof(uint32));
		
		geom.SetMaterial("testDiffuseSpec");

		if (type == eLayerRoads)
			geom.SetMaterial("$default");

		Entity* entity = new Entity(name);
		MeshComponent* meshcomp = entity->CreateComponent<MeshComponent>();
		meshcomp->m_DrawableFlags.Set(Drawable::eLightMap);

		CMesh* mesh = CreateMeshFromGeoms(name, &geom, 1);
		meshcomp->SetMesh(mesh, MeshComponent::eMeshDelete);

		entity->SetPos(CVec3(0, 10, -10));
		gScene.AddEntity(entity);

		LOG("Read: %s, type: %d, nv: %d, ni: %d\n", name, type, nv, ni);
	}
	
	return true;
}

