#include "window_system.h"
#include "input_system.h"

#if defined(VSR_GRAPHICS_API_VULKAN)
#define GLFW_INCLUDE_VULKAN
#endif
#include "GLFW/glfw3.h"

#include <iostream>
#include <cassert>

namespace Visor
{
	static WindowSystem* pInstance = nullptr;

	WindowSystem::Window& WindowSystem::getWindow()
	{
		return _window;
	}

	WindowSystem::WindowSystem(ui32 width, ui32 height)
		: _window(width, height, "visor")
	{}

	void WindowSystem::pollEvents()
	{
		glfwPollEvents();
	}

	void WindowSystem::start(ui32 width, ui32 height)
	{
		assert(pInstance == nullptr);

		if (glfwInit() == 0)
		{
			std::cerr << "could not initialize GLFW\n";
			std::exit(EXIT_FAILURE);
		}

		pInstance = new WindowSystem(width, height);
	}

	void WindowSystem::terminate()
	{
		assert(pInstance != nullptr);
		delete pInstance;
		pInstance = nullptr;

		glfwTerminate();
	}

	WindowSystem& WindowSystem::getInstance()
	{
		assert(pInstance != nullptr);
		return *pInstance;
	}

	static void keyCallback(GLFWwindow* pWindow, i32 glfwKey, i32 scancode, i32 action, i32 mode)
	{
		InputSystem::Key key = InputSystem::Key::COUNT;

		switch(glfwKey)
		{
			case(GLFW_KEY_Q) : key = InputSystem::Key::Q; break;
			case(GLFW_KEY_W) : key = InputSystem::Key::W; break;
			case(GLFW_KEY_E) : key = InputSystem::Key::E; break;
			case(GLFW_KEY_R) : key = InputSystem::Key::R; break;
			case(GLFW_KEY_T) : key = InputSystem::Key::T; break;
			case(GLFW_KEY_S) : key = InputSystem::Key::S; break;
			default : break;
		}

		if(key != InputSystem::Key::COUNT)
		{
			if(action == GLFW_PRESS)
			{
				InputSystem::getInstance().setKeyPressed(key, true);
			}
			else if(action == GLFW_RELEASE)
			{
				InputSystem::getInstance().setKeyPressed(key, false);
			}
		}
	}

	static void mousePositionCallback(GLFWwindow* pWindow, f64 x, f64 y)
	{
		InputSystem::getInstance().setMousePosition((f32)x, (f32)y);
	}
	
	WindowSystem::Window::Window(ui32 width, ui32 height, const std::string& title)
		: _width(width)
		, _height(height)
		, _title(title)
		, _pNativeHandle(nullptr)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		_pNativeHandle = (void*)glfwCreateWindow(_width, _height, "ghoust", NULL, NULL);
		if (_pNativeHandle == NULL)
		{
			std::cerr << "could not create GLFW window\n";
			std::exit(EXIT_FAILURE);
		}

		glfwSetKeyCallback((GLFWwindow*)_pNativeHandle, keyCallback);
		glfwSetCursorPosCallback((GLFWwindow*)_pNativeHandle, mousePositionCallback);
	}

	WindowSystem::Window::~Window()
	{
		glfwDestroyWindow((GLFWwindow*)_pNativeHandle);
	}

	bool WindowSystem::Window::shouldClose()
	{
		return glfwWindowShouldClose((GLFWwindow*)_pNativeHandle);
	}

#if defined(VSR_GRAPHICS_API_VULKAN)
	VkSurfaceKHR WindowSystem::Window::createVkSurface(VkInstance instance, VkAllocationCallbacks* pAllocator)
	{
		VkSurfaceKHR surface;
		
		if(glfwCreateWindowSurface(instance, (GLFWwindow*)_pNativeHandle, pAllocator, &surface) != VK_SUCCESS)
		{
			std::cerr << "could not create vulkan surface\n";
			std::exit(EXIT_FAILURE);
		}

		return surface;
	}

	std::vector<const c8*> WindowSystem::Window::getRequiredVkInstanceExtensions()
	{
		uint32_t requiredExtensionCount;
		const c8** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
		return std::vector<const c8*>(requiredExtensions, requiredExtensions + requiredExtensionCount);
	}
#endif

	ui32 WindowSystem::Window::getWidth() const
	{
		return _width;
	}
	
	ui32 WindowSystem::Window::getHeight() const
	{
		return _height;
	}
	
	const std::string& WindowSystem::Window::getTitle() const
	{
		return _title;
	}
}