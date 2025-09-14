#pragma once

#include "types.h"
#include "maths.h"
#include "AABB.h"

namespace Visor
{
	struct Ray
	{
	public:
		Ray(const Vector3<f32>& position, const Vector3<f32>& direction);

		b8 getIntersection(const AABB& AABB, Vector3<f32>& intersection);

	public:
		Vector3<f32> position;
		Vector3<f32> direction;
	};
}