#pragma once
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
// Converts given lat/lon in WGS84 Datum to XY in Spherical Mercator EPSG:900913"
//-----------------------------------------------------------------------------
Vec2d LonLatToMeters(const Vec2d& _lonLat);
//-----------------------------------------------------------------------------
// Converts XY point from Spherical Mercator EPSG:900913 to lat/lon in WGS84 Datum
//-----------------------------------------------------------------------------
Vec2d MetersToLongLat(const Vec2d& m);
//-----------------------------------------------------------------------------
// Returns tile for given mercator coordinates
//-----------------------------------------------------------------------------
Vec2i MetersToTile(const Vec2d& m, const int zoom);
//-----------------------------------------------------------------------------
// Returns bounds of the given tile in EPSG:900913 coordinates
//-----------------------------------------------------------------------------
Vec2d TileMin(const Vec2i& t, const int zoom);
Vec2d TileMax(const Vec2i& t, const int zoom);
Vec2d TileMin(const Vec3i& t);
Vec2d TileMax(const Vec3i& t);

Vec4d TileBounds(const Vec2i& t, const int zoom);
//-----------------------------------------------------------------------------
// Converts TMS tile coordinates to Google Tile coordinates
//-----------------------------------------------------------------------------
Vec2i TmsToGoogleTile(const Vec2i& t, const int zoom);