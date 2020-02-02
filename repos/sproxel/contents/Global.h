#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <ImathColor.h>
#include "VoxelGridGroup.h"

enum SproxelAxis { X_AXIS, Y_AXIS, Z_AXIS };

enum SproxelTool { TOOL_SPLAT,
                   TOOL_FLOOD,
                   TOOL_RAY,
                   TOOL_DROPPER,
                   TOOL_ERASER,
                   TOOL_REPLACE,
                   TOOL_SLAB,
                   TOOL_LINE };

#endif
