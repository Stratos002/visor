#include "AABB.h"

#include <cassert>

namespace Visor
{
	AABB::AABB(const Vector3<f32>& minimum, const Vector3<f32>& maximum)
		: minimum(minimum)
		, maximum(maximum)
	{
		assert(minimum.x <= maximum.x && minimum.y <= maximum.y && minimum.z <= maximum.z);
	}
}
