#include "Precompiled.h"
#include "mapping.h"
#include "projection.h"
//-----------------------------------------------------------------------------
const char* layerNames[eNumLayerTypes + 1] =
{
	"unknown",
	"terrain",
	"water",
	"buildings",
	"places",
	"transit",
	"pois",
	"boundaries",
	"roads",
	"earth",
	"landuse",

	NULL		// For IndexFromStringTable
};
//-----------------------------------------------------------------------------
Crgba kindColors[NUM_KINDS];
//-----------------------------------------------------------------------------
void InitializeColors()
{
	for (int i = 0; i < NUM_KINDS; i++)
	{
		kindColors[i] = gRGBA_Red;

		// Buildings
		if (i >= eKindBuilding && i <= eKindAddress)
		{
			kindColors[i] = Crgba(180, 180, 180, 255);
		}
		// Roads
		else if (i >= eKindHighway && i <= eKindPortageway)
		{
			kindColors[i].Random();// = Crgba(80, 80, 60, 255);
		}
		// Landuse
		else if (KIND_IS_LANDUSE(i))
		{
			kindColors[i].Random();// = Crgba(128, 200, 32, 255);
		}
		// Water
		else if (i >= eWaterBasin && i <= eWaterWater)
		{
			kindColors[i] = Crgba(32, 128, 190, 255);
		}
		else
		{
			kindColors[i].Random();
		}
	}
}
//-----------------------------------------------------------------------------
Tile::Tile(int x, int y, int z) : x(x), y(y), z(z)
{
	Vec4d bounds = TileBounds(Vec2i(x, y), z);
	tileOrigin = bounds.xy();
	borders = 0;
}