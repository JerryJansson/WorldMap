#pragma once
#include "..\..\..\..\Source\Modules\GameObject\Entity.h"
#include "projection.h"
#include <bitset>

#define USE_GLM

#ifdef USE_GLM
	#include "glm/glm.hpp"
typedef glm::dvec2	v2d;
typedef glm::vec2	v2;
typedef glm::vec3	v3;
typedef glm::dvec4	v4d;
#define myAbs		glm::abs
#define myNormalize glm::normalize
#define myCross		glm::cross
#define myClamp		glm::clamp
#else
typedef CVec2d	v2d;
typedef CVec2	v2;
typedef CVec3	v3;
typedef CVec4d	v4d;
#define myAbs Abs
#define myNormalize Normalize
#define myCross cross
#define myClamp Clamp
#endif

//-----------------------------------------------------------------------------
enum Border {
	right,
	left,
	bottom,
	top,
};
//-----------------------------------------------------------------------------
struct Tile {
	int x;
	int y;
	int z;

	std::bitset<4> borders;
	Vec2d tileOrigin;

	bool operator==(const Tile& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

	Tile(int x, int y, int z);
};

/*namespace std {
	template<>
	struct hash<Tile> {
		size_t operator()(const Tile &tile) const {
			return std::hash<int>()(tile.x) ^ std::hash<int>()(tile.y) ^ std::hash<int>()(tile.z);
		}
	};
}*/
//-----------------------------------------------------------------------------
enum ELayerType
{
	eLayerUnknown,		// 0

	eLayerTerrain,		// 1
	eLayerWater,		// 2
	eLayerBuildings,	// 3
	eLayerPlaces,		// 4
	eLayerTransit,		// 5
	eLayerPois,			// 6
	eLayerBoundaries,	// 7
	eLayerRoads,		// 8
	eLayerEarth,		// 9
	eLayerLanduse,		// 10

	eNumLayerTypes
};
//-----------------------------------------------------------------------------
enum EFeatureKind
{
	eKindUnknown,

	// Buildings
	eKindBuilding,
	eKindBuildingPart,
	eKindAddress,

	// Roads
	eKindHighway,
	eKindMajorRoad,
	eKindMinorRoad,
	eKindRail,
	eKindPath,
	eKindFerry,
	eKindPiste,
	eKindAerialway,
	eKindAeroway,
	eKindRacetrack,
	eKindPortageway,

	// Landuse
	eKindAerodrome,
	eKindAttraction,
	eKindBeach,
	eKindBridge,
	eKind_fence,
	eKindForest,
	eKindGarden,
	eKindGolfcourse,
	eKindGrass,
	eKindPark,
	eKindPedestrian,
	eKindRailway,
	eKindRecreationground,
	eKindResidential,
	eKind_retaining_wall,

	// Water
	eWaterBasin,
	eWaterBay,
	eWaterCanal,
	eWaterDitch,
	eWaterDock,
	eWaterDrain,
	eWaterFjord,
	eWaterLake,
	eWaterOcean,
	eWaterPlaya,
	eWaterRiver,
	eWaterRiverbank,
	eWaterSea,
	eWaterStream,
	eWaterStrait,
	eWaterSwimming_pool,
	eWaterWater,

	NUM_KINDS
};
//-----------------------------------------------------------------------------
extern const char* layerNames[eNumLayerTypes + 1];
//-----------------------------------------------------------------------------
bool GetTile(class MyTile* t, TArray<GGeom>& geoms);