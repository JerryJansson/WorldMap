#pragma once
#include "..\..\..\..\Source\Modules\GameObject\Entity.h"
#include "projection.h"

//#define USE_GLM

#ifdef USE_GLM
	#include "glm/glm.hpp"
typedef glm::dvec2	v2d;
typedef glm::vec2	v2;
typedef glm::vec3	v3;
typedef glm::vec4	v4;
typedef glm::dvec4	v4d;
#define myAbs		glm::abs
#define myNormalize glm::normalize
#define myCross		glm::cross
#define myClamp		glm::clamp
#define myDot		glm::dot
#define myLength	glm::length
#else
typedef Vec2d	v2d;
typedef Vec2	v2;
typedef Vec3	v3;
typedef Vec4	v4;
typedef Vec4d	v4d;
#define myAbs Abs
#define myNormalize Normalize
#define myCross cross
#define myClamp Clamp
#define myDot dot
#define myLength length
#endif

//-----------------------------------------------------------------------------
enum Border {
	right,
	left,
	bottom,
	top,
};
typedef vtxP4NC vtxMap;
//-----------------------------------------------------------------------------
struct StreamResult : ListNode<StreamResult>
{
	class MyTile* tile;
	TArray<struct StreamGeom*> geoms;
};
//-----------------------------------------------------------------------------
bool GetTile(StreamResult* result);