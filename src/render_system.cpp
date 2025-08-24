#include "render_system.h"
#include "window_system.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#include "render_system_backend_vk.h"
#endif

#include <cassert>

namespace Visor
{
	static RenderSystem* pInstance = nullptr;

	void RenderSystem::render(const Camera& camera)
	{
		assert(pInstance != nullptr);
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RenderSystemBackendVk::getInstance().render(camera);
		#endif
	}

	void RenderSystem::start(const std::vector<Entity>& entities)
	{
		assert(pInstance == nullptr);
		pInstance = new RenderSystem();
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RenderSystemBackendVk::start(entities);
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