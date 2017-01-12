#pragma once

// http://www.codepool.biz/build-use-libcurl-vs2015-windows.html
// https://mapzen.com/documentation/vector-tiles/layers/

/* Tile process:

vectiler()
	//-----------------------------------------------------------------------------
	// Fills TileVectorData which is only an array of Layers
	//-----------------------------------------------------------------------------
	DownloadVectorTile()
		ParseJSON -> doc
		//-----------------------------------------------------------------------------
		// Builds Layers. Fills features with properties (height, kind, etc..) &
		// 3d points for points, lines, polygons
		//-----------------------------------------------------------------------------
		for(layers in doc)
			GeoJson::extractLayer(layer)
				for(features)
					GeoJson::extractFeature()
	//-----------------------------------------------------------------------------
	// We have all json data in TileVectorData->Layers->Features
	// Extrude, triangulate every feature into a PolygonMesh
	// A PolygonMesh contains 3d PN, indices, layertype & featurekind
	//-----------------------------------------------------------------------------
	for(layers)
		for(features)
			arr.Add = CreateMeshFromFeature(feature)

*/

struct Params2
{
	const char* apiKey;
	int			tilex, tiley, tilez;
	bool		terrain;
	int			terrainSubdivision;
	float		terrainExtrusionScale;
	bool		vectorData;
};

bool vectiler(const Params2& params);
