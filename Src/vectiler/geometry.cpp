#include "Precompiled.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "geometry.h"
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
	for (auto vertex : mesh.vertices)
		file << "vn " << vertex.normal.x << " " << vertex.normal.z << " " << -vertex.normal.y << "\n";
	//for (auto vertex : mesh.vertices)
	//	file << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
}
//-----------------------------------------------------------------------------
void addPositions(std::ostream& file, const PolygonMesh& mesh, float offsetx, float offsety)
{
	// JJ - flip
	for (auto vertex : mesh.vertices)
	{
	file << "v " << vertex.position.x + offsetx + mesh.offset.x << " "
	<< vertex.position.z << " "
	<< -vertex.position.y + offsety + mesh.offset.y
	<< "\n";
	}
	/*for (auto vertex : mesh.vertices)
	{
		file << "v " << vertex.position.x + offsetx + mesh.offset.x << " "
			<< vertex.position.y + offsety + mesh.offset.y << " "
			<< vertex.position.z << "\n";
	}*/
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
	std::vector<std::unique_ptr<PolygonMesh>>& meshes,
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

				for (const auto& mesh : meshes) {
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