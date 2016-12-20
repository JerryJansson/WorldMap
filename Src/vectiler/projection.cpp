#include "Precompiled.h"
#include "projection.h"

#define D_PI 3.1415926535897932384626433832795

// http://www.maptiler.org/google-maps-coordinates-tile-bounds-projection/
// Initialize the TMS Global Mercator pyramid
const double dTileSize = 256.0;
const double dEarthRad = 6378137.0;
const double initialResolution = 2.0 * D_PI * dEarthRad / dTileSize; // 156543.03392804062 for tileSize 256 pixels
const double originShift = D_PI * dEarthRad;						 // 20037508.342789244
const double halfCircumference = originShift;
const double inv180 = 1.0 / 180.0;
const double inv360 = 1.0 / 360.0;
//-----------------------------------------------------------------------------
// Converts given lat/lon in WGS84 Datum to XY in Spherical Mercator EPSG:900913
//-----------------------------------------------------------------------------
Vec2d LonLatToMeters(const Vec2d& _lonLat)
{
	Vec2d m;
	m.x = _lonLat.x * originShift * inv180;
	m.y = log(tan(D_PI * 0.25 + _lonLat.y * D_PI * inv360)) * dEarthRad;
	return m;
}
//-----------------------------------------------------------------------------
// Converts XY point from Spherical Mercator EPSG:900913 to lat/lon in WGS84 Datum
//-----------------------------------------------------------------------------
Vec2d MetersToLongLat(const Vec2d& m)
{
	Vec2d ll;
	ll.x = (m.x / originShift) * 180.0;
	ll.y = (m.y / originShift) * 180.0;
	ll.y = 180 / D_PI * (2.0 * atan(exp(ll.y * D_PI / 180.0)) - D_PI / 2.0);
	return ll;
}
//-----------------------------------------------------------------------------
// Converts pixel coordinates in given zoom level of pyramid to EPSG:900913
//-----------------------------------------------------------------------------
Vec2d PixelsToMeters(const Vec2d& p, const int zoom)
{
	const double res = initialResolution / (1 << zoom);
	return Vec2d(p.x * res - originShift, p.y * res - originShift);
}
//-----------------------------------------------------------------------------
// Converts EPSG:900913 to pyramid pixel coordinates in given zoom level
//-----------------------------------------------------------------------------
Vec2d MetersToPixels(const Vec2d& m, const int zoom)
{
	const double res = initialResolution / (1 << zoom);
	Vec2d p;
	p.x = (m.x + originShift) / res;
	p.y = (m.y + originShift) / res;
	return p;
}
//-----------------------------------------------------------------------------
// Returns a tile covering region in given pixel coordinates
//-----------------------------------------------------------------------------
Vec2i PixelsToTile(const Vec2d& p)
{
	Vec2i t;
	t.x = int(ceil(p.x / dTileSize)) - 1;
	t.y = int(ceil(p.y / dTileSize)) - 1;
	return t;
}
//-----------------------------------------------------------------------------
// Move the origin of pixel coordinates to top-left corner
//-----------------------------------------------------------------------------
/*	def PixelsToRaster(self, px, py, zoom) :
	mapSize = self.tileSize << zoom
	return px, mapSize - py
*/
//-----------------------------------------------------------------------------
// Returns tile for given mercator coordinates
//-----------------------------------------------------------------------------
Vec2i MetersToTile(const Vec2d& m, const int zoom)
{
	const Vec2d p = MetersToPixels(m, zoom);
	return PixelsToTile(p);
}
//-----------------------------------------------------------------------------
// Returns lower left corner of the given tile in EPSG:900913 coordinates
//-----------------------------------------------------------------------------
Vec2d TileMin(const Vec2i& t, const int zoom) { return PixelsToMeters(Vec2d(t.x * dTileSize, t.y * dTileSize), zoom);				}
Vec2d TileMax(const Vec2i& t, const int zoom) { return PixelsToMeters(Vec2d((t.x + 1) * dTileSize, (t.y + 1) * dTileSize), zoom);	}
//-----------------------------------------------------------------------------
// Returns bounds of the given tile in EPSG:900913 coordinates
//-----------------------------------------------------------------------------
Vec4d TileBounds(const Vec2i& t, const int zoom)
{
	const Vec2d min = PixelsToMeters(Vec2d(t.x * dTileSize, t.y * dTileSize), zoom);
	const Vec2d max = PixelsToMeters(Vec2d((t.x + 1) * dTileSize, (t.y + 1) * dTileSize), zoom);
	return Vec4d(min, max);
}
//-----------------------------------------------------------------------------
// Returns bounds of the given tile in latutude/longitude using WGS84 datum
//-----------------------------------------------------------------------------
/*def TileLatLonBounds(self, tx, ty, zoom) :
	bounds = self.TileBounds(tx, ty, zoom)
	minLat, minLon = self.MetersToLatLon(bounds[0], bounds[1])
	maxLat, maxLon = self.MetersToLatLon(bounds[2], bounds[3])
	return (minLat, minLon, maxLat, maxLon)*/

/*def ZoomForPixelSize(self, pixelSize) :
	"Maximal scaledown zoom of the pyramid closest to the pixelSize."
	for i in range(30) :
	if pixelSize > self.Resolution(i) :
	return i - 1 if i != 0 else 0 # We don't want to scale up
	*/
//-----------------------------------------------------------------------------
// Converts TMS tile coordinates to Google Tile coordinates
// coordinate origin is moved from bottom - left to top - left corner of the extent
//-----------------------------------------------------------------------------
Vec2i TmsToGoogleTile(const Vec2i& t, const int zoom)
{
	return Vec2i(t.x, (1 << zoom) - 1 - t.y);
}
//-----------------------------------------------------------------------------
// Switch to TMS Tile representation from Google
//-----------------------------------------------------------------------------
/*v2d ToTmsTile(v2d t, int zoom)
{
	return v2d(t.x, ((int)pow(2, zoom) - 1) - t.y);
}*/