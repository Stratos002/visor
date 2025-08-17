#pragma once

#include "window.h"
#include "camera.h"
#include "entity.h"

#include <vector>

namespace Visor
{
	class Renderer
	{
	public:
		void render(const Camera& camera);

		static void start(Window& window, const std::vector<Entity>& entities);
		static void terminate();
		static Renderer& getInstance();
	};
}