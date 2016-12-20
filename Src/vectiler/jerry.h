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

	double invScale = 0.0;
	Vec2d tileOrigin;

	bool operator==(const Tile& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

	Tile(int x, int y, int z);
};

namespace std {
	template<>
	struct hash<Tile> {
		size_t operator()(const Tile &tile) const {
			return std::hash<int>()(tile.x) ^ std::hash<int>()(tile.y) ^ std::hash<int>()(tile.z);
		}
	};
}

//-----------------------------------------------------------------------------

//int GetTile(const char* tileX, const char* tileY, int tileZ, CStrL& outFileName);
class MyTile* GetTile2(const int tileX, const int tileY, const int zoom);
bool GetTile3(MyTile* t, TArray<GGeom>& geoms);