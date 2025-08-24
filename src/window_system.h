#pragma once

#include "types.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#include <vulkan/vulkan.h>
#endif

#include <string>
#include <vector>

namespace Visor
{
	/*
	this system can manage only 1 window for now, so for now, the only window
	is created when this system starts. Later, we should be
	able to access window data and open/close them using a handle
	*/
	class WindowSystem
	{
	public:
		// not sure if this class should even exist or be accessible. Later, window data should be directly accessed through a handle
		class Window
		{
		public:
			Window(ui32 width, ui32 height, const std::string& title);
			~Window();

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
		
	public:
		void pollEvents();
	
		static void start(ui32 width, ui32 height);
		static void terminate();
		static WindowSystem& getInstance();
	
		Window& getWindow();

	private:
		WindowSystem(ui32 width, ui32 height);

	private:
		Window _window;
	};
}