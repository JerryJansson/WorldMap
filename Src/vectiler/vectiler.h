#ifndef VECTILER_H
#define VECTILER_H

// JJ
// http://www.codepool.biz/build-use-libcurl-vs2015-windows.html

struct Params {
    const char* apiKey;
    const char* tilex;
    const char* tiley;
    int tilez;
    float offset[2];
    bool splitMesh;
    int aoSizeHint;
    int aoSamples;
    bool aoBaking;
    bool append;
    bool terrain;
    int terrainSubdivision;
    float terrainExtrusionScale;
    bool buildings;
    float buildingsExtrusionScale;
    bool roads;
    float roadsHeight;
    float roadsExtrusionWidth;
    bool normals;
    float buildingsHeight;
    int pedestal;
    float pedestalHeight;
};

int vectiler(struct Params parameters, CStrL& outFileName);

struct Params2 {
	const char* apiKey;
	int			tilex, tiley, tilez;
	bool		terrain;
	int			terrainSubdivision;
	float		terrainExtrusionScale;
	bool		buildings;
	float		buildingsHeight;
	float		buildingsExtrusionScale;
	bool		roads;
	float		roadsHeight;
	float		roadsExtrusionWidth;
	//int pedestal;
	//float pedestalHeight;
};

bool vectiler2(Params2 params);



#endif
