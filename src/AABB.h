#pragma once

#include "types.h"
#include "maths.h"

namespace Visor
{
	struct AABB
	{
	public:
		AABB(const Vector3<f32>& minimum, const Vector3<f32>& maximum);

	public:
		Vector3<f32> minimum;
		Vector3<f32> maximum;
	};
}
