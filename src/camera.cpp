#include "camera.h"

namespace Visor
{
	void Camera::lookAt(const Vector3<f32>& position)
	{
		Vector3<f32> direction = position - this->position;
		direction.normalize();
		yaw = std::atan2(-direction.x, direction.z);
		pitch = std::asin(direction.y);
	}
}
