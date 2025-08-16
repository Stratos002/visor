#pragma once

#include "types.h"
#include "math.h"
#include "mesh.h"

namespace Visor
{
	class Entity
	{
	public:
		Entity(const Vector3<f32>& position, f32 yaw, f32 pitch, f32 roll, const Mesh& mesh);

		const Mesh& getMesh() const;

	public:
		Vector3<f32> position;
		f32 yaw;
		f32 pitch;
		f32 roll;

	private:
		Mesh _mesh;
	};
}