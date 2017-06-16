#include "Precompiled.h"
#include "mapping.h"
#include "projection.h"
//-----------------------------------------------------------------------------
Crgba kindColors[NUM_KINDS];
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
void InitializeColors()
{
	for (int i = 0; i < NUM_KINDS; i++)
	{
		Crgba& c = kindColors[i];
		c = gRGBA_Red;

		// Buildings
		if (i >= eKind_building && i <= eKind_address)
		{
			c = Crgba(217, 208, 201, 255);
		}
		// Transit
		else if (i >= eKind_light_rail && i <= eKind_tram)
		{
			c.Random();
		}
		// Roads
		else if (i >= eKind_highway && i <= eKind_rail)//eKindPortageway)
		{
			if (i == eKind_highway)			c = Crgba(241, 188, 198);
			else if (i == eKind_major_road) c = Crgba(252, 214, 164);
			else if (i == eKind_minor_road) c = Crgba(255, 255, 255);
			else if (i == eKind_path)		c = Crgba(154, 154, 154);
			else
			{
				int abba = 10;
			}
			//else c.Random();// = Crgba(80, 80, 60, 255);
		}
		// Landuse
		else if (i >= eKind_aerodrome && i <= eKind_zoo)
		{
			switch (i)
			{
			case eKind_park: c = Crgba(200, 250, 204); break;
			}
		}
		// Water
		else if (i >= eKind_basin && i <= eKind_water)
		{
			c = Crgba(32, 128, 190, 255);
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