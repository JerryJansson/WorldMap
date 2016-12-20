#include "Precompiled.h"
#include "projection.h"
//-----------------------------------------------------------------------------
const double dTileSize = 256.0;
const int iTileSize = 256;
const double inv_tileSize = 1.0 / dTileSize;
#define R_EARTH 6378137.0
#define PI M_PI
constexpr static double INV_360 = 1.0 / 360.0;
constexpr static double INV_180 = 1.0 / 180.0;
constexpr static double HALF_CIRCUMFERENCE = PI * R_EARTH;
//-----------------------------------------------------------------------------
v2d lonLatToMeters(const v2d _lonLat)
{
    v2d meters;
    meters.x = _lonLat.x * HALF_CIRCUMFERENCE * INV_180;
    meters.y = log(tan(PI * 0.25 + _lonLat.y * PI * INV_360)) * (double)R_EARTH;
    return meters;
}
//-----------------------------------------------------------------------------
v2d pixelsToMeters(const v2d _pix, const int _zoom)
{
	v2d meters;
	double res = (2.0 * HALF_CIRCUMFERENCE * inv_tileSize) / (1 << _zoom);
	meters.x = _pix.x * res - HALF_CIRCUMFERENCE;
	meters.y = _pix.y * res - HALF_CIRCUMFERENCE;
	return meters;
}
//-----------------------------------------------------------------------------
glm::dvec4 tileBounds(int x, int y, int z)
{
    return glm::dvec4(
        pixelsToMeters({ x * iTileSize, y * iTileSize }, z),
        pixelsToMeters({ (x + 1) * iTileSize, (y + 1) * iTileSize }, z)
    );
}

//-----------------------------------------------------------------------------
v2d tileCenter(int x, int y, int z)
{
    return pixelsToMeters(v2d(x * iTileSize + iTileSize * 0.5, (y * iTileSize + iTileSize * 0.5)), z);
}


// JJ

// lat / lon in WGS84 Datum
// Spherical Mercator EPSG:900913 (meters)

//SOURCE: http://stackoverflow.com/questions/12896139/geographic-coordinates-converter
const int EarthRadius = 6378137;
const double InitialResolution = 2 * M_PI * EarthRadius / dTileSize;
const double OriginShift = 2 * M_PI * EarthRadius / 2;
//-----------------------------------------------------------------------------
// Converts given lat/lon in WGS84 Datum to XY in Spherical Mercator EPSG:900913
//-----------------------------------------------------------------------------
v2d LatLonToMeters(double lat, double lon)
{
	v2d p;
	p.x = (lon * OriginShift / 180);
	p.y = (log(tan((90 + lat) * PI / 360)) / (PI / 180));
	p.y = (p.y * OriginShift / 180);
	return p;
}
//-----------------------------------------------------------------------------
v2d LatLonToMeters(const v2d v)
{
	return LatLonToMeters(v.x, v.y);
}
//-----------------------------------------------------------------------------
//Resolution (meters/pixel) for given zoom level (measured at Equator)
//-----------------------------------------------------------------------------
double Resolution(int zoom)
{
	return InitialResolution / (pow(2, zoom));
}
//-----------------------------------------------------------------------------
// Converts pixel coordinates in given zoom level of pyramid to EPSG:900913
//-----------------------------------------------------------------------------
v2d PixelsToMeters(const v2d p, const int zoom)
{
	double res = Resolution(zoom);
	v2d met;
	met.x = (p.x * res - OriginShift);
	met.y = -(p.y * res - OriginShift);
	return met;
}
//-----------------------------------------------------------------------------
// Converts EPSG:900913 to pyramid pixel coordinates in given zoom level
//-----------------------------------------------------------------------------
v2d MetersToPixels(const v2d m, int zoom)
{
	double res = Resolution(zoom);
	v2d pix;
	pix.x = ((m.x + OriginShift) / res);
	pix.y = ((-m.y + OriginShift) / res);
	return pix;
}
//-----------------------------------------------------------------------------
// Returns a TMS (NOT Google!) tile covering region in given pixel coordinates
//-----------------------------------------------------------------------------
v2d PixelsToTile(const v2d p)
{
	v2d t;
	t.x = (int)ceil(p.x / dTileSize) - 1;
	t.y = (int)ceil(p.y / dTileSize) - 1;
	return t;
}
//-----------------------------------------------------------------------------
v2d PixelsToRaster(const v2d p, int zoom)
{
	int mapSize = iTileSize << zoom;
	return v2d(p.x, mapSize - p.y);
}
//-----------------------------------------------------------------------------
// Returns tile for given mercator coordinates
//-----------------------------------------------------------------------------
v2d MetersToTile(v2d m, int zoom)
{
	v2d p = MetersToPixels(m, zoom);
	return PixelsToTile(p);
}
//-----------------------------------------------------------------------------
// Returns bounds of the given tile in EPSG:900913 coordinates
//-----------------------------------------------------------------------------
v4d TileBoundsInMeters(v2d t, int zoom)
{
	v2d min = PixelsToMeters(v2d(t.x * dTileSize, t.y * dTileSize), zoom);
	v2d max = PixelsToMeters(v2d((t.x + 1) * dTileSize, (t.y + 1) * dTileSize), zoom);
	return v4d(min, max);// -min);
}
//-----------------------------------------------------------------------------
v2d MetersToLatLon(const v2d m)
{
	v2d ll;
	ll.x = (m.x / OriginShift) * 180.0f;
	ll.y = (m.y / OriginShift) * 180.0f;
	ll.y = 180.0f / PI * (2.0f * atan(exp(ll.y * PI / 180.0f)) - PI / 2.0f);
	return ll;
}
//-----------------------------------------------------------------------------
//Returns bounds of the given tile in latutude/longitude using WGS84 datum
//public static RectD TileLatLonBounds(Vector2d t, int zoom)
//{
//    var bound = TileBounds(t, zoom);
//    var min = MetersToLatLon(new Vector2d(bound.Min.x, bound.Min.y));
//    var max = MetersToLatLon(new Vector2d(bound.Min.x + bound.Size.x, bound.Min.y + bound.Size.y));
//    return new RectD(min.x, min.y, Math.Abs(max.x - min.x), Math.Abs(max.y - min.y));
//}

//-----------------------------------------------------------------------------
double ZoomForPixelSize(double pixelSize)
{
	for (int i = 0; i < 30; i++)
	{
		if (pixelSize > Resolution(i))
			return i != 0 ? i - 1 : 0;
	}
	//throw new InvalidOperationException();
	return 0;
}
//-----------------------------------------------------------------------------
// Switch to Google Tile representation from TMS
//-----------------------------------------------------------------------------
v2d ToGoogleTile(v2d t, int zoom)
{
	return v2d(t.x, ((int)pow(2, zoom) - 1) - t.y);
}
//-----------------------------------------------------------------------------
// Switch to TMS Tile representation from Google
//-----------------------------------------------------------------------------
v2d ToTmsTile(v2d t, int zoom)
{
	return v2d(t.x, ((int)pow(2, zoom) - 1) - t.y);
}