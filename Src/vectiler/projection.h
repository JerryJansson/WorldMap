#pragma once
#include "jerry.h"
#include <cmath>
#include <functional>
#include <bitset>

/* 
	Longitude growths at E, decreases to W (x-axis)
	Latitude growths to N, decreases to S (y-axis)

	Google XYZ zoom is between [0,21]

	Google 0,0,0 have
		Longitude -180 - +180
		Latitude -85.05 - + 85.05	(0 at equator)
		Mercator x: -20M - +20M
				 y: -20M - +20M		(0 at equator)

	Google 36059, 19267, 16 (Stockholm Stadion)
		Long, Lat	: <18.078, 59.344> - 
					  <18.083, 59.347>
		Mercator	: <2012434.08, 8255199.05> - 
					  <2013045.57, 8255810.55>
	Google 36059, 19268, 16 (Stockholm Stadion, 1 below)
		 Long, Lat	: <18.078, 59.341>
					  <18.083, 59.344>
		 Mercator	: <2012434.08, 8254587.55> -
					  <2013045.57, 8255199.05>

		Diff long,lat	:	<0, -0.03>
		Diff mercator	:	<0, -611,5>
	
	Each tile is 256 pixels by 256 pixels.
	Zoom level 0 is 1 tile. (1 x 1)
	Zoom level 1 is 4 tiles. (2 x 2)
	Zoom level 2 is 16 tiles. (4 x 4)
	Zoom level 3 is 64 tiles. (8 x 8)
	Zoom level 4 is 256 tiles(16 x 16)
	Zoom level 16 is (65536 x 65536)
	Zoom level n is (2^n x 2^n tiles)
*/
//-----------------------------------------------------------------------------
//extern const int iTileSize;
//-----------------------------------------------------------------------------
Vec2d lonLatToMeters(const Vec2d& _lonLat);
v2d pixelsToMeters(const v2d _pix, const int _zoom);
v4d tileBounds(int x, int y, int z);
v2d tileCenter(int x, int y, int z);

//Vec4d TileBounds2(const int x, const int y, const int zoom);
//-----------------------------------------------------------------------------
v2d LatLonToMeters(double lat, double lon);
Vec4d TileBoundsInMeters(const Vec2i& t, const int zoom);
Vec2i MetersToTile(const Vec2d& m, const int zoom);
v2d MetersToLonLat(const v2d& m);

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
