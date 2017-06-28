#pragma once
#include "../../../../Source/Modules/GameObject/Entity.h"
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
//-----------------------------------------------------------------------------
enum DiscCache
{
	eDiscNoCache,		// No allowed to use any cached tiles at all
	eDiscJsonCache,		// Allowed to use cached json network tiles
	eDiscBinaryCache	// Allowed to use binary cached geometry
};
#if JJ_WORLDMAP == 1
//-----------------------------------------------------------------------------
struct StreamResult : ListNode<StreamResult>
{
	class MyTile* tile;
	TArray<struct StreamGeom*> geoms;
};
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
	bool GetTile(StreamResult* result);
#endif

#if JJ_WORLDMAP == 2
	bool GetTile(class TileData* t);
#endif
