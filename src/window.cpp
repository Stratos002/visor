#include "window.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#define GLFW_INCLUDE_VULKAN
#endif
#include "GLFW/glfw3.h"

#include <iostream>

namespace Visor
{
	Window::Window(ui32 width, ui32 height, const std::string& title)
		: _width(width)
		, _height(height)
		, _title(title)
		, _pNativeHandle(nullptr)
	{}

	Window::~Window()
	{}

	void Window::open()
	{
		if (glfwInit() == 0)
		{
			std::cerr << "could not initialize GLFW\n";
			std::exit(EXIT_FAILURE);
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_pNativeHandle = (void*)glfwCreateWindow(_width, _height, "ghoust", NULL, NULL);
		if (_pNativeHandle == NULL)
		{
			std::cerr << "could not create GLFW window\n";
			std::exit(EXIT_FAILURE);
		}
	}

	void Window::close()
	{
		glfwDestroyWindow((GLFWwindow*)_pNativeHandle);
		glfwTerminate();
	}
		
	void Window::pollEvents()
	{
		glfwPollEvents();
	}

	bool Window::shouldClose()
	{
		return glfwWindowShouldClose((GLFWwindow*)_pNativeHandle);
	}

#if defined(VSR_GRAPHICS_API_VULKAN)
	VkSurfaceKHR Window::createVkSurface(VkInstance instance, VkAllocationCallbacks* pAllocator)
	{
		VkSurfaceKHR surface;
		
		if(glfwCreateWindowSurface(instance, (GLFWwindow*)_pNativeHandle, pAllocator, &surface) != VK_SUCCESS)
		{
			std::cerr << "could not create vulkan surface\n";
			std::exit(EXIT_FAILURE);
		}

		return surface;
	}

	std::vector<const c8*> Window::getRequiredVkInstanceExtensions()
	{
		uint32_t requiredExtensionCount;
		const c8** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
		return std::vector<const c8*>(requiredExtensions, requiredExtensions + requiredExtensionCount);
	}
#endif

	ui32 Window::getWidth() const
	{
		return _width;
	}
	
	ui32 Window::getHeight() const
	{
		return _height;
	}
	
	const std::string& Window::getTitle() const
	{
		return _title;
	}
}