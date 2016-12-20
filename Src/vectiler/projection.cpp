#include "Precompiled.h"
#include "projection.h"
//-----------------------------------------------------------------------------
v2d lonLatToMeters(const v2d _lonLat) {
    v2d meters;
    meters.x = _lonLat.x * HALF_CIRCUMFERENCE * INV_180;
    meters.y = log(tan(PI * 0.25 + _lonLat.y * PI * INV_360)) * (double)R_EARTH;
    return meters;
}
//-----------------------------------------------------------------------------
glm::dvec4 tileBounds(int x, int y, int z, double _tileSize)
{
    return glm::dvec4(
        pixelsToMeters({ x * _tileSize, y * _tileSize }, z, 1.0 / _tileSize),
        pixelsToMeters({ (x + 1) * _tileSize, (y + 1) * _tileSize }, z, 1.0 / _tileSize)
    );
}
//-----------------------------------------------------------------------------
v2d pixelsToMeters(const v2d _pix, const int _zoom, double _invTileSize)
{
    v2d meters;
    double res = (2.0 * HALF_CIRCUMFERENCE * _invTileSize) / (1 << _zoom);
    meters.x = _pix.x * res - HALF_CIRCUMFERENCE;
    meters.y = _pix.y * res - HALF_CIRCUMFERENCE;
    return meters;
}
//-----------------------------------------------------------------------------
v2d tileCenter(int x, int y, int z, double _tileSize)
{
    return pixelsToMeters(v2d(x * _tileSize + _tileSize * 0.5,
                                    (y * _tileSize + _tileSize * 0.5)),
                                     z, 1.0 / _tileSize);
}