#include "renderer_backend_vk.h"

#define VOLK_IMPLEMENTATION
#include "volk.h"

#include <iostream>
#include <cassert>

namespace Visor
{
	static RendererBackendVk* pInstance = nullptr;

	void RendererBackendVk::render(const Camera& camera, const std::vector<Entity>& entities)
	{
		assert(pInstance != nullptr);

		vkWaitForFences(_device, 1, &_commandBufferExecutedFence, VK_FALSE, UINT64_MAX);
		vkResetFences(_device, 1, &_commandBufferExecutedFence);

		ui32 availableSwapchainImageIndex = 0;
		if (vkAcquireNextImageKHR(_device, _swapchain, UINT64_MAX, _imageAvailableSemaphore, VK_NULL_HANDLE, &availableSwapchainImageIndex) != VK_SUCCESS)
		{
			std::cerr << "could not acquire next swapchain image index\n";
			std::exit(EXIT_FAILURE);
		}

		vkResetCommandBuffer(_commandBuffer, 0);

		VkCommandBufferBeginInfo commandBufferBeginInfo = {};
		commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBufferBeginInfo.pNext = nullptr;
		commandBufferBeginInfo.flags = 0;
		commandBufferBeginInfo.pInheritanceInfo = nullptr;
		vkBeginCommandBuffer(_commandBuffer, &commandBufferBeginInfo);

		VkClearColorValue clearColor = {};
		clearColor.float32[0] = 1.0f;
		clearColor.float32[1] = 0.0f;
		clearColor.float32[2] = 0.0f;
		clearColor.float32[3] = 1.0f;

		VkImageSubresourceRange imageSubresourceRange = {};
		imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageSubresourceRange.baseMipLevel = 0;
		imageSubresourceRange.levelCount = 1;
		imageSubresourceRange.baseArrayLayer = 0;
		imageSubresourceRange.layerCount = 1;

		vkCmdClearColorImage(_commandBuffer, _swapchainImages[availableSwapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageSubresourceRange);

		vkEndCommandBuffer(_commandBuffer);

		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &_imageAvailableSemaphore;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_commandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &_imageRenderedSemaphores[availableSwapchainImageIndex];

		if (vkQueueSubmit(_queue, 1, &submitInfo, _commandBufferExecutedFence) != VK_SUCCESS)
		{
			std::cerr << "could not sumbit command buffer\n";
			std::exit(EXIT_FAILURE);
		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &_imageRenderedSemaphores[availableSwapchainImageIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &_swapchain;
		presentInfo.pImageIndices = &availableSwapchainImageIndex;

		vkQueuePresentKHR(_queue, &presentInfo);
	}

	void RendererBackendVk::start(Window& window)
	{
		assert(pInstance == nullptr);
		pInstance = new RendererBackendVk(window);
	}

	void RendererBackendVk::terminate()
	{
		assert(pInstance != nullptr);
		delete pInstance;
		pInstance = nullptr;
	}

	RendererBackendVk& RendererBackendVk::getInstance()
	{
		assert(pInstance != nullptr);
		return *pInstance;
	}

	RendererBackendVk::RendererBackendVk(Window& window)
		: _pAllocator(nullptr)
	{
		if (volkInitialize() != VK_SUCCESS)
		{
			std::cerr << "could not initialize volk\n";
			std::exit(EXIT_FAILURE);
		}

		_instance = createInstance(window.getTitle(), window.getTitle(), window.getRequiredVkInstanceExtensions(), _pAllocator);
		volkLoadInstance(_instance);
		_surface = window.createVkSurface(_instance, _pAllocator);
		_physicalDevice = pickPhysicalDevice(_instance);
		_queueFamilyIndex = findQueueFamilyIndex(_physicalDevice, _surface);
		_device = createDevice(_queueFamilyIndex, _physicalDevice, _pAllocator);
		vkGetDeviceQueue(_device, _queueFamilyIndex, 0, &_queue);
		{
			_swapchainFormat = VK_FORMAT_R8G8B8A8_UNORM;
			_swapchain = createSwapchain(_physicalDevice, _swapchainFormat, window.getWidth(), window.getHeight(), _device, _surface, _queueFamilyIndex, _pAllocator);
			ui32 swapchainImageCount = 0;
			vkGetSwapchainImagesKHR(_device, _swapchain, &swapchainImageCount, nullptr);
			_swapchainImages.resize(swapchainImageCount);
			vkGetSwapchainImagesKHR(_device, _swapchain, &swapchainImageCount, _swapchainImages.data());
			for (const VkImage& swapchainImage : _swapchainImages)
			{
				_swapchainImageViews.push_back(createImageView(swapchainImage, _swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, _device, _pAllocator));
			}
		}
		_commandPool = createCommandPool(_queueFamilyIndex, _device, _pAllocator);
		_descriptorPool = createDescriptorPool(_device, _pAllocator);
		_imageAvailableSemaphore = createSemaphore(_device, _pAllocator);

		for (ui32 swapchainImageIndex = 0; swapchainImageIndex < _swapchainImages.size(); ++swapchainImageIndex)
		{
			_imageRenderedSemaphores.push_back(createSemaphore(_device, _pAllocator));
		}
		_commandBufferExecutedFence = createFence(_device, _pAllocator);
		_commandBuffer = allocateCommandBuffer(_commandPool, _device);
	}

	RendererBackendVk::~RendererBackendVk()
	{
		vkDeviceWaitIdle(_device);

		vkDestroyFence(_device, _commandBufferExecutedFence, _pAllocator);
		vkDestroySemaphore(_device, _imageAvailableSemaphore, _pAllocator);
		for (ui32 swapchainImageIndex = 0; swapchainImageIndex < _swapchainImages.size(); ++swapchainImageIndex)
		{
			vkDestroySemaphore(_device, _imageRenderedSemaphores[swapchainImageIndex], _pAllocator);
			vkDestroyImageView(_device, _swapchainImageViews[swapchainImageIndex], _pAllocator);
		}
		vkDestroySwapchainKHR(_device, _swapchain, _pAllocator);

		vkDestroyDescriptorPool(_device, _descriptorPool, _pAllocator);
		vkDestroyCommandPool(_device, _commandPool, _pAllocator);
		vkDestroyDevice(_device, _pAllocator);
		vkDestroySurfaceKHR(_instance, _surface, _pAllocator);
		vkDestroyInstance(_instance, _pAllocator);
	}

	VkInstance RendererBackendVk::createInstance(
		const std::string& applicationName,
		const std::string& engineName,
		const std::vector<const c8*>& requiredExtensionNames,
		const VkAllocationCallbacks* pAllocator)
	{
		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = NULL;
		applicationInfo.pApplicationName = applicationName.data();
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pEngineName = engineName.data();
		applicationInfo.apiVersion = VK_MAKE_API_VERSION(1, 4, 0, 0);

		std::vector<const c8*> layerNames;
		layerNames.push_back("VK_LAYER_KHRONOS_validation");

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = NULL;
		instanceCreateInfo.flags = 0;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledLayerCount = (ui32)layerNames.size();
		instanceCreateInfo.ppEnabledLayerNames = layerNames.data();
		instanceCreateInfo.enabledExtensionCount = (ui32)requiredExtensionNames.size();
		instanceCreateInfo.ppEnabledExtensionNames = requiredExtensionNames.data();

		VkInstance instance;
		if (vkCreateInstance(&instanceCreateInfo, pAllocator, &instance) != VK_SUCCESS)
		{
			std::cerr << "could not create VkInstance\n";
			std::exit(EXIT_FAILURE);
		}

		return instance;
	}

	VkPhysicalDevice RendererBackendVk::pickPhysicalDevice(VkInstance instance)
	{
		ui32 physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL);

		if (physicalDeviceCount == 0)
		{
			std::cerr << "no VkPhysicalDevice found\n";
			std::exit(EXIT_FAILURE);
		}

		std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

		VkPhysicalDeviceProperties physicalDeviceProperties;
		for (VkPhysicalDevice physicalDevice : physicalDevices)
		{
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				std::cout << "discrete GPU found : " << physicalDeviceProperties.deviceName << "\n";
				return physicalDevice;
			}
		}

		vkGetPhysicalDeviceProperties(physicalDevices[0], &physicalDeviceProperties);
		std::cout << "no discrete GPU found, fallback GPU : " << physicalDeviceProperties.deviceName << "\n";
		return physicalDevices[0];
	}

	ui32 RendererBackendVk::findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		ui32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

		std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		for (ui32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, &presentSupport);

			if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
				queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT &&
				queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_TRANSFER_BIT &&
				presentSupport)
			{
				return queueFamilyIndex;
			}
		}

		std::cerr << "no suitable queue family found\n";
		std::exit(EXIT_FAILURE);

		return -1;
	}

	VkDevice RendererBackendVk::createDevice(ui32 queueFamilyIndex, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* pAllocator)
	{
		float queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = NULL;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		std::vector<const c8*> extensionNames;
		extensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

		VkPhysicalDeviceFeatures deviceFeatures = {};
		deviceFeatures.multiDrawIndirect = VK_TRUE;
		deviceFeatures.vertexPipelineStoresAndAtomics = VK_TRUE;
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = NULL;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = NULL;
		deviceCreateInfo.enabledExtensionCount = 1;
		deviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

		VkDevice device;
		if (vkCreateDevice(physicalDevice, &deviceCreateInfo, pAllocator, &device) != VK_SUCCESS)
		{
			std::cerr << "could not create device\n";
			std::exit(EXIT_FAILURE);
		}

		return device;
	}

	VkSwapchainKHR RendererBackendVk::createSwapchain(
		VkPhysicalDevice physicalDevice,
		VkFormat format,
		ui32 width,
		ui32 height,
		VkDevice device,
		VkSurfaceKHR surface,
		ui32 queueFamilyIndex,
		const VkAllocationCallbacks* pAllocator)
	{
		VkBool32 surfaceSupported;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface, &surfaceSupported);
		if (surfaceSupported == VK_FALSE)
		{
			std::cerr << "surface is not supported by physical device\n";
			std::exit(EXIT_FAILURE);
		}

		// ===== present modes =====
		ui32 availablePresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &availablePresentModeCount, NULL);
		std::vector<VkPresentModeKHR> availablePresentModes(availablePresentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &availablePresentModeCount, availablePresentModes.data());

		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (ui32 availablePresentModeIndex = 0; availablePresentModeIndex < availablePresentModeCount; ++availablePresentModeIndex)
		{
			if (availablePresentModes[availablePresentModeIndex] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		// ===== min image count =====
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities);

		ui32 minImageCount = 2;
		if (minImageCount < surfaceCapabilities.minImageCount)
		{
			minImageCount = surfaceCapabilities.minImageCount;
		}

		if (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount)
		{
			minImageCount = surfaceCapabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.pNext = NULL;
		swapchainCreateInfo.flags = 0;
		swapchainCreateInfo.surface = surface;
		swapchainCreateInfo.minImageCount = minImageCount; // TODO: query this!
		swapchainCreateInfo.imageFormat = format;
		swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		swapchainCreateInfo.imageExtent.width = width;
		swapchainCreateInfo.imageExtent.height = height;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // better performances, and we only have 1 queue family anyway
		swapchainCreateInfo.queueFamilyIndexCount = 1;
		swapchainCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
		swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

		VkSwapchainKHR swapchain;
		if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, pAllocator, &swapchain) != VK_SUCCESS)
		{
			std::cerr << "could not create swapchain\n";
			std::exit(EXIT_FAILURE);
		}

		return swapchain;
	}

	VkDescriptorPool RendererBackendVk::createDescriptorPool(VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkDescriptorPoolSize poolSizes[5];
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[0].descriptorCount = 100; // TODO harcodded crap

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[1].descriptorCount = 100; // TODO harcodded crap

		poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[2].descriptorCount = 10000; // TODO harcodded crap

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = 100; // TODO harcodded crap

		poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSizes[4].descriptorCount = 100; // TODO harcodded crap

		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.pNext = NULL;
		descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptorPoolCreateInfo.maxSets = 10000; // TODO harcodded crap
		descriptorPoolCreateInfo.poolSizeCount = sizeof(poolSizes) / sizeof(poolSizes[0]);
		descriptorPoolCreateInfo.pPoolSizes = &poolSizes[0];

		VkDescriptorPool descriptorPool;
		if (vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, pAllocator, &descriptorPool) != VK_SUCCESS)
		{
			std::cerr << "could not create descriptor pool\n";
			std::exit(EXIT_FAILURE);
		}

		return descriptorPool;
	}

	VkDescriptorSet RendererBackendVk::allocateDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, VkDevice device)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = NULL;
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;

		VkDescriptorSet descriptorSet;
		if (vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet) != VK_SUCCESS)
		{
			std::cerr << "could not allocate descriptor set\n";
			std::exit(EXIT_FAILURE);
		}

		return descriptorSet;
	}

	VkCommandPool RendererBackendVk::createCommandPool(ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = NULL;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;

		VkCommandPool commandPool;
		if (vkCreateCommandPool(device, &commandPoolCreateInfo, pAllocator, &commandPool) != VK_SUCCESS)
		{
			std::cerr << "could not create command pool\n";
			std::exit(EXIT_FAILURE);
		}

		return commandPool;
	}

	VkCommandBuffer RendererBackendVk::allocateCommandBuffer(VkCommandPool commandPool, VkDevice device)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = NULL;
		commandBufferAllocateInfo.commandPool = commandPool;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocateInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		if (vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer) != VK_SUCCESS)
		{
			std::cerr << "could not allocate command buffer\n";
			std::exit(EXIT_FAILURE);
		}

		return commandBuffer;
	}

	VkBuffer RendererBackendVk::createBuffer(ui32 size, VkBufferUsageFlags usage, ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = NULL;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 1;
		bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;

		VkBuffer buffer;
		if (vkCreateBuffer(device, &bufferCreateInfo, pAllocator, &buffer) != VK_SUCCESS)
		{
			std::cerr << "could not create buffer\n";
			std::exit(EXIT_FAILURE);
		}

		return buffer;
	}

	VkImage RendererBackendVk::createImage(
		VkFormat format,
		ui32 width,
		ui32 height,
		VkImageUsageFlags usage,
		ui32 queueFamilyIndex,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkImageCreateInfo imageCreateInfo = {};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = NULL;
		imageCreateInfo.flags = 0;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent.width = width;
		imageCreateInfo.extent.height = height;
		imageCreateInfo.extent.depth = 1;
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.queueFamilyIndexCount = 1;
		imageCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImage image;
		if (vkCreateImage(device, &imageCreateInfo, pAllocator, &image) != VK_SUCCESS)
		{
			std::cerr << "could not create image\n";
			std::exit(EXIT_FAILURE);
		}

		return image;
	}

	VkImageView RendererBackendVk::createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = NULL;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		if (vkCreateImageView(device, &imageViewCreateInfo, pAllocator, &imageView) != VK_SUCCESS)
		{
			std::cerr << "could not create image view\n";
			std::exit(EXIT_FAILURE);
		}

		return imageView;
	}

	ui32 RendererBackendVk::findMemoryTypeIndex(
		VkPhysicalDevice physicalDevice,
		ui32 compatibleMemoryTypeBits,
		VkMemoryPropertyFlags memoryPropertyFlags)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (ui32 availableMemoryTypeIndex = 0; availableMemoryTypeIndex < memoryProperties.memoryTypeCount; ++availableMemoryTypeIndex)
		{
			if ((compatibleMemoryTypeBits & (1 << availableMemoryTypeIndex)) &&
				(memoryProperties.memoryTypes[availableMemoryTypeIndex].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
			{
				return availableMemoryTypeIndex;
			}
		}

		std::cerr << "no suitable memory type found\n";
		std::exit(EXIT_FAILURE);

		return -1;
	}

	VkDeviceMemory RendererBackendVk::allocateDeviceMemoryForBuffer(
		VkDevice device,
		VkBuffer buffer,
		VkPhysicalDevice physicalDevice,
		VkMemoryPropertyFlags memoryPropertyFlags,
		const VkAllocationCallbacks* pAllocator)
	{
		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = NULL;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

		VkDeviceMemory deviceMemory;
		if (vkAllocateMemory(device, &memoryAllocateInfo, pAllocator, &deviceMemory) != VK_SUCCESS)
		{
			std::cerr << "could not allocate memory for buffer\n";
			std::exit(EXIT_FAILURE);
		}

		return deviceMemory;
	}

	VkDeviceMemory RendererBackendVk::allocateDeviceMemoryForImage(
		VkDevice device,
		VkImage image,
		VkPhysicalDevice physicalDevice,
		VkMemoryPropertyFlags memoryPropertyFlags,
		const VkAllocationCallbacks* pAllocator)
	{
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, image, &memoryRequirements);

		VkMemoryAllocateInfo memoryAllocateInfo = {};
		memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memoryAllocateInfo.pNext = NULL;
		memoryAllocateInfo.allocationSize = memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, memoryPropertyFlags);

		VkDeviceMemory deviceMemory;
		if (vkAllocateMemory(device, &memoryAllocateInfo, pAllocator, &deviceMemory) != VK_SUCCESS)
		{
			std::cerr << "could not allocate memory for image\n";
			std::exit(EXIT_FAILURE);
		}

		return deviceMemory;
	}

	VkDescriptorSetLayout RendererBackendVk::createDescriptorSetLayout(
		const std::vector<VkDescriptorSetLayoutBinding>& bindings,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = NULL;
		descriptorSetLayoutCreateInfo.flags = 0;
		descriptorSetLayoutCreateInfo.bindingCount = (ui32)bindings.size();
		descriptorSetLayoutCreateInfo.pBindings = bindings.data();

		VkDescriptorSetLayout descriptorSetLayout;
		if (vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, pAllocator, &descriptorSetLayout) != VK_SUCCESS)
		{
			std::cerr << "could not create descriptor set layout\n";
			std::exit(EXIT_FAILURE);
		}

		return descriptorSetLayout;
	}

	VkPipelineLayout RendererBackendVk::createPipelineLayout(
		ui32 descriptorSetLayoutCount,
		const VkDescriptorSetLayout* pDescriptorSetLayouts,
		ui32 pushConstantRangeCount,
		const VkPushConstantRange* pPushConstantRanges,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = NULL;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayoutCount;
		pipelineLayoutCreateInfo.pSetLayouts = pDescriptorSetLayouts;
		pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRangeCount;
		pipelineLayoutCreateInfo.pPushConstantRanges = pPushConstantRanges;

		VkPipelineLayout pipelineLayout;
		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, pAllocator, &pipelineLayout) != VK_SUCCESS)
		{
			std::cerr << "could not create pipeline layout\n";
			std::exit(EXIT_FAILURE);
		}

		return pipelineLayout;
	}

	VkShaderModule RendererBackendVk::createShaderModule(
		ui32 size,
		const ui32* pCode,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.pNext = NULL;
		shaderModuleCreateInfo.flags = 0;
		shaderModuleCreateInfo.codeSize = size;
		shaderModuleCreateInfo.pCode = pCode;

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &shaderModuleCreateInfo, pAllocator, &shaderModule) != VK_SUCCESS)
		{
			std::cerr << "could not create shader module\n";
			std::exit(EXIT_FAILURE);
		}

		return shaderModule;
	}

	VkPipeline RendererBackendVk::createComputePipeline(
		VkShaderModule shaderModule,
		VkPipelineLayout pipelineLayout,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
		shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStageCreateInfo.flags = 0;
		shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		shaderStageCreateInfo.module = shaderModule;
		shaderStageCreateInfo.pName = "main";
		shaderStageCreateInfo.pSpecializationInfo = NULL;

		VkComputePipelineCreateInfo computePipelineCreateInfo = {};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = NULL;
		computePipelineCreateInfo.flags = 0;
		computePipelineCreateInfo.stage = shaderStageCreateInfo;
		computePipelineCreateInfo.layout = pipelineLayout;
		computePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		computePipelineCreateInfo.basePipelineIndex = 0;

		VkPipeline computePipeline;
		if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, pAllocator, &computePipeline) != VK_SUCCESS)
		{
			std::cerr << "could not create compute pipeline\n";
			std::exit(EXIT_FAILURE);
		}

		return computePipeline;
	}

	VkPipeline RendererBackendVk::createGraphicsPipeline(
		VkShaderModule vertexShaderModule,
		VkShaderModule fragmentShaderModule,
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo,
		ui32 viewportWidth,
		ui32 viewportHeight,
		bool enableDepthWrite,
		VkPipelineLayout pipelineLayout,
		VkRenderPass renderPass,
		ui32 subpassIndex,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
		vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageCreateInfo.pNext = NULL;
		vertexShaderStageCreateInfo.flags = 0;
		vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageCreateInfo.module = vertexShaderModule;
		vertexShaderStageCreateInfo.pName = "main";
		vertexShaderStageCreateInfo.pSpecializationInfo = NULL;

		VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
		fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageCreateInfo.pNext = NULL;
		fragmentShaderStageCreateInfo.flags = 0;
		fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageCreateInfo.module = fragmentShaderModule;
		fragmentShaderStageCreateInfo.pName = "main";
		fragmentShaderStageCreateInfo.pSpecializationInfo = NULL;

		VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
			vertexShaderStageCreateInfo,
			fragmentShaderStageCreateInfo
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.pNext = NULL;
		inputAssemblyStateCreateInfo.flags = 0;
		inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)viewportWidth;
		viewport.height = (float)viewportHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = viewportWidth;
		scissor.extent.height = viewportHeight;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.pNext = NULL;
		viewportStateCreateInfo.flags = 0;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCreateInfo.pNext = NULL;
		rasterizationStateCreateInfo.flags = 0;
		rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
		rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE;
		rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
		rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
		rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
		rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
		rasterizationStateCreateInfo.lineWidth = 1.0;

		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
		multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCreateInfo.pNext = NULL;
		multisampleStateCreateInfo.flags = 0;
		multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleStateCreateInfo.minSampleShading = 0.0f;
		multisampleStateCreateInfo.pSampleMask = NULL;
		multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
		depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCreateInfo.pNext = NULL;
		depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
		depthStencilStateCreateInfo.depthWriteEnable = (VkBool32)enableDepthWrite;
		depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
		depthStencilStateCreateInfo.minDepthBounds = 0.0f;
		depthStencilStateCreateInfo.maxDepthBounds = 1.0f;

		VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
		colorBlendAttachmentState.blendEnable = VK_FALSE;
		colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachmentState.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
		colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCreateInfo.pNext = NULL;
		colorBlendStateCreateInfo.flags = 0;
		colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
		colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
		colorBlendStateCreateInfo.attachmentCount = 1;
		colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
		colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
		colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

		/*
	typedef struct VkGraphicsPipelineCreateInfo {
		VkStructureType                                  sType;
		const void*                                      pNext;
		VkPipelineCreateFlags                            flags;
		ui32                                         stageCount;
		const VkPipelineShaderStageCreateInfo*           pStages;
		const VkPipelineVertexInputStateCreateInfo*      pVertexInputState;
		const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
		const VkPipelineTessellationStateCreateInfo*     pTessellationState;
		const VkPipelineViewportStateCreateInfo*         pViewportState;
		const VkPipelineRasterizationStateCreateInfo*    pRasterizationState;
		const VkPipelineMultisampleStateCreateInfo*      pMultisampleState;
		const VkPipelineDepthStencilStateCreateInfo*     pDepthStencilState;
		const VkPipelineColorBlendStateCreateInfo*       pColorBlendState;
		const VkPipelineDynamicStateCreateInfo*          pDynamicState;
		VkPipelineLayout                                 layout;
		VkRenderPass                                     renderPass;
		ui32                                         subpass;
		VkPipeline                                       basePipelineHandle;
		int32_t                                          basePipelineIndex;
	} VkGraphicsPipelineCreateInfo;

		*/

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.pNext = NULL;
		graphicsPipelineCreateInfo.flags = 0;
		graphicsPipelineCreateInfo.stageCount = sizeof(shaderStageCreateInfos) / sizeof(shaderStageCreateInfos[0]);
		graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos;
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pTessellationState = NULL;
		graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
		graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState = NULL;
		graphicsPipelineCreateInfo.layout = pipelineLayout;
		graphicsPipelineCreateInfo.renderPass = renderPass;
		graphicsPipelineCreateInfo.subpass = subpassIndex;
		graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
		graphicsPipelineCreateInfo.basePipelineIndex = 0;

		VkPipeline graphicsPipeline;
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, pAllocator, &graphicsPipeline) != VK_SUCCESS)
		{
			std::cerr << "could not create graphics pipeline\n";
			std::exit(EXIT_FAILURE);
		}

		return graphicsPipeline;
	}

	VkFence RendererBackendVk::createFence(VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = NULL;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkFence fence;
		if (vkCreateFence(device, &fenceCreateInfo, pAllocator, &fence) != VK_SUCCESS)
		{
			std::cerr << "could not create fence\n";
			std::exit(EXIT_FAILURE);
		}

		return fence;
	}

	VkSemaphore RendererBackendVk::createSemaphore(VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = NULL;
		semaphoreCreateInfo.flags = 0;

		VkSemaphore semaphore;
		if (vkCreateSemaphore(device, &semaphoreCreateInfo, pAllocator, &semaphore) != VK_SUCCESS)
		{
			std::cerr << "could not create semaphore\n";
			std::exit(EXIT_FAILURE);
		}

		return semaphore;
	}
}