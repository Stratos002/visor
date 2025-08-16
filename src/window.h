#pragma once

#include "types.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#include <vulkan/vulkan.h>
#endif

#include <string>
#include <vector>

namespace Visor
{
	class Window
	{
	public:
		Window(ui32 width, ui32 height, const std::string& title);
		~Window();

		void open();
		void close();
		void pollEvents();
		bool shouldClose();
#if defined(VSR_GRAPHICS_API_VULKAN)
		VkSurfaceKHR createVkSurface(VkInstance instance, VkAllocationCallbacks* pAllocator);
		std::vector<const c8*> getRequiredVkInstanceExtensions();
#endif

		ui32 getWidth() const;
		ui32 getHeight() const;
		const std::string& getTitle() const;

	private:
		ui32 _width;
		ui32 _height;
		std::string _title;
		void* _pNativeHandle;
	};
}