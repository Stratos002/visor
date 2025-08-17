#include "renderer.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#include "renderer_backend_vk.h"
#endif

#include <cassert>

namespace Visor
{
	static Renderer* pInstance = nullptr;

	void Renderer::render(const Camera& camera)
	{
		assert(pInstance != nullptr);
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RendererBackendVk::getInstance().render(camera);
		#endif
	}

	void Renderer::start(Window& window, const std::vector<Entity>& entities)
	{
		assert(pInstance == nullptr);
		pInstance = new Renderer();
		#if defined(VSR_GRAPHICS_API_VULKAN)
			RendererBackendVk::start(window, entities);
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