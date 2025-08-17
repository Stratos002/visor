#pragma once

#include "types.h"
#include "maths.h"

namespace Visor
{
	struct Camera
	{
		f32 fov;
		Vector3<f32> position;
		f32 yaw;
		f32 pitch;
		f32 roll;
	};
}