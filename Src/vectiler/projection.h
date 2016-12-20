#pragma once

//#include "glm/glm.hpp"
#include "jerry.h"
#include <cmath>
#include <functional>
#include <bitset>

//-----------------------------------------------------------------------------
v2d lonLatToMeters(const v2d _lonLat);
v2d pixelsToMeters(const v2d _pix, const int _zoom);
v4d tileBounds(int x, int y, int z);
v2d tileCenter(int x, int y, int z);
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
    v2d tileOrigin;

    bool operator==(const Tile& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

    Tile(int x, int y, int z) : x(x), y(y), z(z)
	{
        v4d bounds = tileBounds(x, y, z);
        tileOrigin = v2d(0.5 * (bounds.x + bounds.z), -0.5 * (bounds.y + bounds.w));
        
		// JJ
		//double scale = 0.5 * myAbs(bounds.x - bounds.z);
        //invScale = 1.0 / scale;
		invScale = 1.0f;

        borders = 0;
    }
};
//-----------------------------------------------------------------------------
namespace std {
    template<>
    struct hash<Tile> {
        size_t operator()(const Tile &tile) const {
            return std::hash<int>()(tile.x) ^ std::hash<int>()(tile.y) ^ std::hash<int>()(tile.z);
        }
    };
}

//-----------------------------------------------------------------------------
v2d lonLatToMeters(const v2d _lonLat);
//-----------------------------------------------------------------------------
v2d LatLonToMeters(double lat, double lon);
v4d TileBoundsInMeters(v2d t, int zoom);
v2d MetersToTile(v2d m, int zoom);
