#pragma once

// http://www.codepool.biz/build-use-libcurl-vs2015-windows.html
// https://mapzen.com/documentation/vector-tiles/layers/

struct Params2
{
	const char* apiKey;
	int			tilex, tiley, tilez;
	bool		terrain;
	int			terrainSubdivision;
	float		terrainExtrusionScale;
	bool		buildings;
	bool		roads;
};

bool vectiler(const Params2& params);
