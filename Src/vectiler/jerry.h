#pragma once
#include "..\..\..\..\Source\Modules\GameObject\Entity.h"
#include "projection.h"

typedef Vec2d	v2d;
typedef Vec2	v2;
typedef Vec3	v3;
typedef Vec4	v4;
typedef Vec4d	v4d;

//-----------------------------------------------------------------------------
enum Border {
	right,
	left,
	bottom,
	top,
};
typedef vtxP4NC vtxMap;

#if JJ_WORLDMAP == 1
//-----------------------------------------------------------------------------
struct StreamResult : ListNode<StreamResult>
{
	class MyTile* tile;
	TArray<struct StreamGeom*> geoms;
};
//-----------------------------------------------------------------------------
bool GetTile(StreamResult* result);
#endif

#if JJ_WORLDMAP == 2

#endif