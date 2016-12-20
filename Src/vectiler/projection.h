#pragma once

//#include "glm/glm.hpp"
#include "jerry.h"
#include <cmath>
#include <functional>
#include <bitset>
//-----------------------------------------------------------------------------
#define R_EARTH 6378137.0
#define PI M_PI
constexpr static double INV_360 = 1.0 / 360.0;
constexpr static double INV_180 = 1.0 / 180.0;
constexpr static double HALF_CIRCUMFERENCE = PI * R_EARTH;
//-----------------------------------------------------------------------------
v2d lonLatToMeters(const v2d _lonLat);
v2d pixelsToMeters(const v2d _pix, const int _zoom, double _invTileSize);
v4d tileBounds(int x, int y, int z, double _tileSize);
v2d tileCenter(int x, int y, int z, double _tileSize);
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
        v4d bounds = tileBounds(x, y, z, 256.0);
        tileOrigin = v2d(0.5 * (bounds.x + bounds.z), -0.5 * (bounds.y + bounds.w));
        double scale = 0.5 * myAbs(bounds.x - bounds.z);
        invScale = 1.0 / scale;
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
