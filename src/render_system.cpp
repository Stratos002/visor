#include "render_system.h"
#include "window_system.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#include "render_system_backend_vk.h"
#endif

#include <cassert>

namespace Visor
{
	static RenderSystem* pInstance = nullptr;

	void RenderSystem::render(const Camera& camera, const std::vector<Entity>& entities)
	{
		assert(pInstance != nullptr);
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RenderSystemBackendVk::getInstance().render(camera, entities);
		#endif
	}

	void RenderSystem::start()
	{
		assert(pInstance == nullptr);
		pInstance = new RenderSystem();
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RenderSystemBackendVk::start();
		#endif
	}

	void RenderSystem::terminate()
	{
		assert(pInstance != nullptr);
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RenderSystemBackendVk::terminate();
		#endif
		delete pInstance;
		pInstance = nullptr;
	}

	RenderSystem& RenderSystem::getInstance()
	{
		assert(pInstance != nullptr);
		return *pInstance;
	}
}