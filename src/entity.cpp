#include "entity.h"

namespace Visor
{
	Entity::Entity(const Vector3<f32>& position, f32 yaw, f32 pitch, f32 roll, const Mesh& mesh)
		: position(position)
		, yaw(yaw)
		, pitch(pitch)
		, roll(roll)
		, _mesh(mesh)
	{}

	const Mesh& Entity::getMesh() const
	{
		return _mesh;
	}
}