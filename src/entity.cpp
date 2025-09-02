#include "entity.h"

namespace Visor
{
	Entity::Entity(const Vector3<f32>& position, f32 scaleX, f32 scaleY, f32 scaleZ, f32 yaw, f32 pitch, f32 roll, const Mesh& mesh)
		: position(position)
		, scaleX(scaleX)
		, scaleY(scaleY)
		, scaleZ(scaleZ)
		, yaw(yaw)
		, pitch(pitch)
		, roll(roll)
		, _mesh(mesh)
	{}

	void Entity::lookAt(const Vector3<f32>& position)
	{
		Vector3<f32> direction = position - this->position;
		direction.normalize();
		yaw = std::atan2(-direction.x, direction.z);
		pitch = std::asin(direction.y);
	}

	const Mesh& Entity::getMesh() const
	{
		return _mesh;
	}
}