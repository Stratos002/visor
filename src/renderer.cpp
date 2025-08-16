#include "renderer.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#include "renderer_backend_vk.h"
#endif

#include <cassert>

namespace Visor
{
	static Renderer* pInstance = nullptr;

	void Renderer::render(const Camera& camera, const std::vector<Entity>& entities)
	{
		assert(pInstance != nullptr);
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RendererBackendVk::getInstance().render(camera, entities);
		#endif
	}

	void Renderer::start(Window& window)
	{
		assert(pInstance == nullptr);
		pInstance = new Renderer();
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RendererBackendVk::start(window);
		#endif
	}

	void Renderer::terminate()
	{
		assert(pInstance != nullptr);
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RendererBackendVk::terminate();
		#endif
		delete pInstance;
		pInstance = nullptr;
	}

	Renderer& Renderer::getInstance()
	{
		assert(pInstance != nullptr);
		return *pInstance;
	}
}