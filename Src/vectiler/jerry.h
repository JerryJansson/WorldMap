#pragma once
#include "..\..\..\..\Source\Modules\GameObject\Entity.h"
#include "projection.h"

#define USE_GLM

#ifdef USE_GLM
	#include "glm/glm.hpp"
typedef glm::dvec2	v2d;
typedef glm::vec2	v2;
typedef glm::vec3	v3;
typedef glm::dvec4	v4d;
#define myAbs		glm::abs
#define myNormalize glm::normalize
#define myCross		glm::cross
#define myClamp		glm::clamp
#else
typedef CVec2d	v2d;
typedef CVec2	v2;
typedef CVec3	v3;
typedef CVec4d	v4d;
#define myAbs Abs
#define myNormalize Normalize
#define myCross cross
#define myClamp Clamp
#endif

//-----------------------------------------------------------------------------
enum Border {
	right,
	left,
	bottom,
	top,
};
typedef vtxPNC vtxMap;
//-----------------------------------------------------------------------------
/*struct StreamResult : ListNode<StreamResult>
{
MyTile* tile;
TArray<GGeom> geoms;
};
MemPoolDynamic<StreamResult> geomPool(8);*/
struct StreamResult : ListNode<StreamResult>
{
	class MyTile* tile;
	TArray<struct StreamGeom*> geoms;
};
//-----------------------------------------------------------------------------
bool GetTile(StreamResult* result);