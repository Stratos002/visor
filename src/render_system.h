#pragma once

#include "camera.h"
#include "entity.h"

#include <vector>

namespace Visor
{
	class RenderSystem
	{
	public:
		void render(const Camera& camera, const std::vector<Entity>& entities);

		static void start();
		static void terminate();
		static RenderSystem& getInstance();
	};
}