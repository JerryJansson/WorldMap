#include "Precompiled.h"
#include "vectiler.h"

int GetTile(const char* tileX, const char* tileY, int tileZ, CStrL& outFileName)
{
	// Default parameters
	int aoAtlasSize = 512;
	int aoSamples = 256;
	int pedestal = 0;
	float pedestalHeight = 0.f;
	int terrainSubdivision = 64;
	float terrainExtrusionScale = 1.f;
	float buildingsExtrusionScale = 1.f;
	float buildingsHeight = 10.0f; //0.0f
	float roadsHeight = 0.2f; // 1.0f
	float roadsExtrusionWidth = 5.f;
	const char* apiKey = "vector-tiles-qVaBcRA";

	// Parse params
	/* flag_usage("[options]");
	flag_string(&name, "name", "File name");
	flag_string(&apiKey, "apikey", "Developer API key (https://mapzen.com/developers/)");
	flag_int(&splitMeshes, "splitMeshes", "Generate one mesh per feature in wavefront file");
	flag_string(&tileX, "tilex", "Tile X (can be a tile range: 19294/19295)");
	flag_string(&tileY, "tiley", "Tile Y (can be a tile range: 24642/24643)");
	flag_int(&tileZ, "tilez", "Tile Z (zoom)");
	flag_float(&offsetX, "offsetx", "Global tile Offset on X coordinate");
	flag_float(&offsetY, "offsety", "Global tile Offset on Y coordinate");
	flag_int(&append, "append", "Append the obj to an existing obj file");
	flag_int(&buildings, "buildings", "Whether to export building geometry");
	flag_float(&buildingsExtrusionScale, "buildingsExtrusionScale", "Building height scale factor");
	flag_float(&buildingsHeight, "buildingsHeight", "The height at which building should be extruded (if no height data is available)");
	flag_int(&pedestal, "pedestal", "Build a pedestal when running with terrain option (Useful for 3d printing)");
	flag_float(&pedestalHeight, "pedestalHeight", "Pedestal height, can be negative");
	flag_int(&terrain, "terrain", "Generate terrain elevation topography");
	flag_int(&terrainSubdivision, "terrainSubdivision", "Terrain mesh subdivision");
	flag_float(&terrainExtrusionScale, "terrainExtrusionScale", "Terrain mesh extrusion scale");
	flag_int(&aoBaking, "aoBaking", "Generate ambiant occlusion baked atlas");
	flag_int(&aoAtlasSize, "aoAtlasSize", "Controls resolution of atlas");
	flag_int(&aoSamples, "aoSamples", "Number of samples for ambient occlusion");
	flag_int(&roads, "roads", "Whether to export roads geometry");
	flag_float(&roadsHeight, "roadsHeight", "The roads height offset (z-axis)");
	flag_float(&roadsExtrusionWidth, "roadsExtrusionWidth", "The roads extrusion width");
	flag_int(&normals, "normals", "Export with normals");
	flag_parse(argc, argv, "v" "0.1.0", 0);*/

	struct Params parameters = 
	{ 
		NULL,						// filename
		"vector-tiles-qVaBcRA",		// apiKey
		tileX,						// Tile X (can be a tile range: 19294/19295)
		tileY,						// Tile Y (can be a tile range: 24642/24643)
		tileZ,						// Tile Z (zoom)
		{ 0.0f, 0.0f },				// Global tile Offset on X/Y coordinate
		false,						// splitMesh. Generate one mesh per feature in wavefront file
		aoAtlasSize, aoSamples, 
		false,						// aoBaking. Generate ambiant occlusion baked atlas
		false,						// Append the obj to an existing obj file
		false,						// terrain. Generate terrain elevation topography
		terrainSubdivision, terrainExtrusionScale, 
		true,						// buildings. Whether to export building geometry
		buildingsExtrusionScale,
		true,						// roads. Whether to export roads geometry
		roadsHeight, roadsExtrusionWidth, 
		true,						// normals. Export with normals
		buildingsHeight, pedestal,
		pedestalHeight };

	if (!parameters.terrain && parameters.pedestal) {
		printf("Pedestal parameters can only be given when exporting with terrain (--terrain)\n");
		return EXIT_FAILURE;
	}

	if (parameters.aoBaking && (parameters.terrain || parameters.roads)) {
		printf("Ambient occlusion baking not yet available when exporting with option --terrain or --roads\n");
		return EXIT_FAILURE;
	}

	return vectiler(parameters, outFileName);
}