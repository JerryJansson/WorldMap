#pragma once
#include "jerry.h"
#include <vector>
#include "mapping.h"
//#define KEEP_2D 1
//-----------------------------------------------------------------------------
#ifdef KEEP_2D
	typedef v2 Point;
#else
	typedef v3 Point;
#endif
typedef std::vector<Point> LineString;
typedef std::vector<LineString> Polygon2;
//-----------------------------------------------------------------------------
enum GeometryType
{
	unknown,
	points,
	lines,
	polygons
};
//-----------------------------------------------------------------------------
struct Feature 
{
    GeometryType geometryType = GeometryType::polygons;

    std::vector<Point> points;
    std::vector<LineString> lineStrings;
    std::vector<Polygon2> polygons;

	// Properties
	float			height = 0;
	float			min_height = 0;
	int64			id = 0;
	CStr			name;
	EFeatureKind	kind = eKind_unknown;
	// 0 - 9: Under everything.Tip : disable earth layer.
	// 190 - 199 : Under water.Above earth and most landuse.
	// 290 - 299 : Under roads.Above borders, water, landuse, and earth.Your classic “underlay”.
	// 490 - 499 : Over all line and polygon features.Under map labels(icons and text), under UI elements(like routeline and search result pins).Your classic raster map overlay.
	int sort_rank = 0;
	bool boundary = false;

	Feature() {}
};
//-----------------------------------------------------------------------------
struct Layer
{
	ELayerType		layerType;
	TArray<Feature> features;
	Layer() {}
	/*Layer(const char* _name)
	{
		layerType = (ELayerType)IndexFromStringTable(_name, layerNames);
		int abba = 10;
	}*/
};
//-----------------------------------------------------------------------------
struct HeightData
{
	std::vector<std::vector<float>> elevation;
	int width;
	int height;
};
#if JJ_WORLDMAP == 1


#endif

#if JJ_WORLDMAP == 2
//-----------------------------------------------------------------------------
class TileData : public Entity
{
public:
	enum EStatus
	{
		eNotLoaded,
		eLoading,
		eLoaded,
		eError
	};

	EStatus						m_Status;
	Vec3i						m_Tms;			// Tms grid coord xyz (z=zoom)
	uint32						m_FrameBump;	// Last frame we used this data
	Vec2d						m_Origo;		// Tile origo (lower left corner) in mercator coordinates
	struct TileNode*			m_Node;
	TArray<struct StreamGeom*>	geoms;

	void Init(const Vec3i& tms)
	{
		m_Status	= eNotLoaded;
		m_Tms		= tms;
		m_Origo		= TileMin(m_Tms);
		const CStrL tileName = Str_Printf("%d_%d_%d", tms.x, tms.y, tms.z);
		SetName(tileName);
	}
	EStatus Status() const				{ return m_Status;						}
	void	SetStatus(EStatus status)	{ m_Status = status;					}
	bool	IsLoaded() const			{ return m_Status == TileData::eLoaded; }
};
//-----------------------------------------------------------------------------
struct TileNode
{
	Vec3i		m_Tms;		// Tms grid coord xyz (z=zoom)
	TileNode*	m_Child[4];
	TileNode*	m_Parent;
	float		m_Dist;
	TileData*	m_Data;
	Caabb		m_NodeAabb;

	TileNode() {}
	bool IsLoaded() const { return m_Data && m_Data->IsLoaded(); }
};
#endif