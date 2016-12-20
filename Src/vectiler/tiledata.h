#pragma once
#include "jerry.h"
#include <vector>
#include <string>
#include <map>
//-----------------------------------------------------------------------------
//struct Properties {
 //   std::map<std::string, double> numericProps;
//};
//-----------------------------------------------------------------------------
enum GeometryType {
    unknown,
    points,
    lines,
    polygons
};
//-----------------------------------------------------------------------------
typedef v3 Point;
typedef std::vector<Point> Line;
typedef std::vector<Line> Polygon2;
//-----------------------------------------------------------------------------
struct Feature 
{
    GeometryType geometryType = GeometryType::polygons;

    std::vector<Point> points;
    std::vector<Line> lines;
    std::vector<Polygon2> polygons;

    //Properties props;
	float height = 0;
	float min_height = 0;
	// 0 - 9: Under everything.Tip : disable earth layer.
	// 190 - 199 : Under water.Above earth and most landuse.
	// 290 - 299 : Under roads.Above borders, water, landuse, and earth.Your classic “underlay”.
	// 490 - 499 : Over all line and polygon features.Under map labels(icons and text), under UI elements(like routeline and search result pins).Your classic raster map overlay.
	int sort_rank = 0;

	// Roads
	float road_width = 0.1f;
};
//-----------------------------------------------------------------------------
struct Layer
{
	ELayerType layerType;
	std::vector<Feature> features;

    //Layer(const std::string& _name) : name(_name) {}
    //std::string name;
	Layer(const char* _name)
	{
		layerType = (ELayerType)IndexFromStringTable(_name, layerNames);
		int abba = 10;
	}
};
//-----------------------------------------------------------------------------
struct TileVectorData {
    std::vector<Layer> layers;
};
//-----------------------------------------------------------------------------
struct HeightData {
	std::vector<std::vector<float>> elevation;
	int width;
	int height;
};