#include "render_system_backend_vk.h"
#include "window_system.h"

#define VOLK_IMPLEMENTATION
#include "volk.h"

#include <cstring>
#include <iostream>
#include <cassert>
#include <cmath>

namespace Visor
{
	static RenderSystemBackendVk* pInstance = nullptr;

	void RenderSystemBackendVk::render(const Camera& camera)
	{
		assert(pInstance != nullptr);

		vkWaitForFences(_device, 1, &_commandBufferExecutedFence, VK_FALSE, UINT64_MAX);
		vkResetFences(_device, 1, &_commandBufferExecutedFence);

		// ==== update global uniform buffer ====
		GlobalUniformBuffer globalUniformBuffer = {};

		globalUniformBuffer.viewProjectionMatrix = Matrix4<f32>::getProjection(camera.fov, _renderArea.extent.width / (f32)_renderArea.extent.height) * Matrix4<f32>::getView(camera.position, camera.yaw, camera.pitch, camera.roll);
		globalUniformBuffer.viewProjectionMatrix.transpose();

		void* pGlobalUniformBufferData = nullptr;
		vkMapMemory(_device, _globalUniformBufferMemory, 0, sizeof(GlobalUniformBuffer), 0, &pGlobalUniformBufferData);
		std::memcpy(pGlobalUniformBufferData, &globalUniformBuffer, sizeof(GlobalUniformBuffer));
		vkUnmapMemory(_device, _globalUniformBufferMemory);

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

		VkClearValue clearColor = {};
		clearColor.color.float32[0] = 0.2f;
		clearColor.color.float32[1] = 0.5f;
		clearColor.color.float32[2] = 0.8f;
		clearColor.color.float32[3] = 1.0f;

		VkRenderingAttachmentInfo colorAttachment = {};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.pNext = nullptr;
		colorAttachment.imageView = _swapchainImageViews[availableSwapchainImageIndex];
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		colorAttachment.resolveImageView = VK_NULL_HANDLE;
		colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue = clearColor;

		VkRenderingInfo renderingInfo = {};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.pNext = nullptr;
		renderingInfo.flags = 0;
		renderingInfo.renderArea = _renderArea;
		renderingInfo.layerCount = 1;
		renderingInfo.viewMask = 0;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = nullptr;
		renderingInfo.pStencilAttachment = nullptr;

		vkCmdBeginRendering(_commandBuffer, &renderingInfo);

		for (const EntityDrawInfo& entityDrawInfo : _entityDrawInfos)
		{
			VkDescriptorSet descriptorSets[] = {
				_globalDescriptorSet,
				entityDrawInfo.entityDescriptorSet
			};

			vkCmdBindDescriptorSets(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entityDrawInfo.graphicsPipelineLayout, 0, 2, descriptorSets, 0, nullptr);
			vkCmdBindPipeline(_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, entityDrawInfo.graphicsPipeline);
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(_commandBuffer, 0, 1, &entityDrawInfo.vertexBuffer, &offset);
			vkCmdBindIndexBuffer(_commandBuffer, entityDrawInfo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(_commandBuffer, entityDrawInfo.indexCount, 1, 0, 0, 0);
		}

		vkCmdEndRendering(_commandBuffer);

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

	void RenderSystemBackendVk::start(const std::vector<Entity>& entities)
	{
		assert(pInstance == nullptr);
		pInstance = new RenderSystemBackendVk(entities);
	}

	void RenderSystemBackendVk::terminate()
	{
		assert(pInstance != nullptr);
		delete pInstance;
		pInstance = nullptr;
	}

	RenderSystemBackendVk& RenderSystemBackendVk::getInstance()
	{
		assert(pInstance != nullptr);
		return *pInstance;
	}

	RenderSystemBackendVk::RenderSystemBackendVk(const std::vector<Entity>& entities)
		: _pAllocator(nullptr)
	{
		if (volkInitialize() != VK_SUCCESS)
		{
			std::cerr << "could not initialize volk\n";
			std::exit(EXIT_FAILURE);
		}

		WindowSystem::Window& window = WindowSystem::getInstance().getWindow();
		_instance = createInstance(window.getTitle(), window.getTitle(), window.getRequiredVkInstanceExtensions(), _pAllocator);
		volkLoadInstance(_instance);
		_surface = window.createVkSurface(_instance, _pAllocator);
		_physicalDevice = pickPhysicalDevice(_instance);
		_queueFamilyIndex = findQueueFamilyIndex(_physicalDevice, _surface);
		_device = createDevice(_queueFamilyIndex, _physicalDevice, _pAllocator);
		vkGetDeviceQueue(_device, _queueFamilyIndex, 0, &_queue);
		{
			_renderArea.offset.x = 0;
			_renderArea.offset.y = 0;
			_renderArea.extent.width = window.getWidth();
			_renderArea.extent.height = window.getHeight();
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

		// ===== frame specific stuff =====

		std::vector<VkDescriptorSetLayoutBinding> globalDescriptorSetLayoutBindings;

		VkDescriptorSetLayoutBinding globalDescriptorSetLayoutBinding = {};
		globalDescriptorSetLayoutBinding.binding = 0;
		globalDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalDescriptorSetLayoutBinding.descriptorCount = 1;
		globalDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

		globalDescriptorSetLayoutBindings.push_back(globalDescriptorSetLayoutBinding);

		_globalDescriptorSetLayout = createDescriptorSetLayout(globalDescriptorSetLayoutBindings, _device, _pAllocator);
		_globalDescriptorSet = allocateDescriptorSet(_descriptorPool, _globalDescriptorSetLayout, _device);

		_globalUniformBuffer = createBuffer(sizeof(GlobalUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, _queueFamilyIndex, _device, _pAllocator);
		_globalUniformBufferMemory = allocateDeviceMemoryForBuffer(_device, _globalUniformBuffer, _physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, _pAllocator);
		vkBindBufferMemory(_device, _globalUniformBuffer, _globalUniformBufferMemory, 0);

		VkDescriptorBufferInfo globalUniformBufferInfo = {};
		globalUniformBufferInfo.buffer = _globalUniformBuffer;
		globalUniformBufferInfo.offset = 0;
		globalUniformBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet globalUniformBufferDescriptorWrite = {};
		globalUniformBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		globalUniformBufferDescriptorWrite.pNext = nullptr;
		globalUniformBufferDescriptorWrite.dstSet = _globalDescriptorSet;
		globalUniformBufferDescriptorWrite.dstBinding = 0;
		globalUniformBufferDescriptorWrite.dstArrayElement = 0;
		globalUniformBufferDescriptorWrite.descriptorCount = 1;
		globalUniformBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalUniformBufferDescriptorWrite.pImageInfo = nullptr;
		globalUniformBufferDescriptorWrite.pBufferInfo = &globalUniformBufferInfo;

		vkUpdateDescriptorSets(_device, 1, &globalUniformBufferDescriptorWrite, 0, nullptr);

		for (const Entity& entity : entities)
		{
			EntityDrawInfo entityDrawInfo = {};
			
			std::vector<VkDescriptorSetLayoutBinding> entityDescriptorSetLayoutBindings;

			VkDescriptorSetLayoutBinding entityUniformBufferDescriptorSetLayoutBinding = {};
			entityUniformBufferDescriptorSetLayoutBinding.binding = 0;
			entityUniformBufferDescriptorSetLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			entityUniformBufferDescriptorSetLayoutBinding.descriptorCount = 1;
			entityUniformBufferDescriptorSetLayoutBinding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

			entityDescriptorSetLayoutBindings.push_back(entityUniformBufferDescriptorSetLayoutBinding);

			entityDrawInfo.entityDescriptorSetLayout = createDescriptorSetLayout(entityDescriptorSetLayoutBindings, _device, _pAllocator);
			entityDrawInfo.entityDescriptorSet = allocateDescriptorSet(_descriptorPool, entityDrawInfo.entityDescriptorSetLayout, _device);

			std::vector<VkDescriptorSetLayout> descriptorSetLayouts = {
				_globalDescriptorSetLayout,
				entityDrawInfo.entityDescriptorSetLayout
			};

			entityDrawInfo.graphicsPipelineLayout = createPipelineLayout(descriptorSetLayouts, 0, nullptr, _device, _pAllocator);

			VkShaderModule vertexShaderModule;
			{
				ui32 codeSize = 0;
				ui32* pCode = nullptr;
				readShader(entity.getMesh().getVertexShaderName(), &codeSize, &pCode);
				vertexShaderModule = createShaderModule(codeSize, pCode, _device, _pAllocator);
			}

			VkShaderModule fragmentShaderModule;
			{
				ui32 codeSize = 0;
				ui32* pCode = nullptr;
				readShader(entity.getMesh().getFragmentShaderName(), &codeSize, &pCode);
				fragmentShaderModule = createShaderModule(codeSize, pCode, _device, _pAllocator);
			}

			VkVertexInputBindingDescription vertexBindingDescription = {};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = sizeof(Mesh::Vertex);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputAttributeDescription vertexAttributeDescription = {};
			vertexAttributeDescription.binding = 0;
			vertexAttributeDescription.format = VK_FORMAT_R32G32B32_SFLOAT;
			vertexAttributeDescription.location = 0;
			vertexAttributeDescription.offset = 0;

			VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
			vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputStateCreateInfo.pNext = nullptr;
			vertexInputStateCreateInfo.flags = 0;
			vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
			vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
			vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
			vertexInputStateCreateInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;

			entityDrawInfo.graphicsPipeline = createGraphicsPipeline(
				vertexShaderModule, 
				fragmentShaderModule, 
				vertexInputStateCreateInfo, 
				_swapchainFormat, 
				VK_FORMAT_D32_SFLOAT, 
				window.getWidth(), 
				window.getHeight(), 
				0, 
				entityDrawInfo.graphicsPipelineLayout, 
				_device, 
				_pAllocator);

			entityDrawInfo.vertexBuffer = createBuffer(sizeof(Mesh::Vertex) * entity.getMesh().getVertices().size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, _queueFamilyIndex, _device, _pAllocator);
			entityDrawInfo.vertexBufferMemory = allocateDeviceMemoryForBuffer(_device, entityDrawInfo.vertexBuffer, _physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, _pAllocator);
			vkBindBufferMemory(_device, entityDrawInfo.vertexBuffer, entityDrawInfo.vertexBufferMemory, 0);

			void* pVertexData = nullptr;
			vkMapMemory(_device, entityDrawInfo.vertexBufferMemory, 0, sizeof(Mesh::Vertex) * entity.getMesh().getVertices().size(), 0, &pVertexData);
			std::memcpy(pVertexData, entity.getMesh().getVertices().data(), sizeof(Mesh::Vertex) * entity.getMesh().getVertices().size());
			vkUnmapMemory(_device, entityDrawInfo.vertexBufferMemory);
			
			entityDrawInfo.indexCount = entity.getMesh().getIndices().size();
			entityDrawInfo.indexBuffer = createBuffer(sizeof(ui32) * entityDrawInfo.indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, _queueFamilyIndex, _device, _pAllocator);
			entityDrawInfo.indexBufferMemory = allocateDeviceMemoryForBuffer(_device, entityDrawInfo.indexBuffer, _physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, _pAllocator);
			vkBindBufferMemory(_device, entityDrawInfo.indexBuffer, entityDrawInfo.indexBufferMemory, 0);

			void* pIndexData = nullptr;
			vkMapMemory(_device, entityDrawInfo.indexBufferMemory, 0, sizeof(ui32) * entityDrawInfo.indexCount, 0, &pIndexData);
			std::memcpy(pIndexData, entity.getMesh().getIndices().data(), sizeof(ui32) * entityDrawInfo.indexCount);
			vkUnmapMemory(_device, entityDrawInfo.indexBufferMemory);

			entityDrawInfo.uniformBuffer = createBuffer(sizeof(EntityUniformBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, _queueFamilyIndex, _device, _pAllocator);
			entityDrawInfo.uniformBufferMemory = allocateDeviceMemoryForBuffer(_device, entityDrawInfo.uniformBuffer, _physicalDevice, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, _pAllocator);
			vkBindBufferMemory(_device, entityDrawInfo.uniformBuffer, entityDrawInfo.uniformBufferMemory, 0);
			
			VkDescriptorBufferInfo entityUniformBufferInfo = {};
			entityUniformBufferInfo.buffer = entityDrawInfo.uniformBuffer;
			entityUniformBufferInfo.offset = 0;
			entityUniformBufferInfo.range = VK_WHOLE_SIZE;


			VkWriteDescriptorSet entityUniformBufferDescriptorWrite = {};
			entityUniformBufferDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			entityUniformBufferDescriptorWrite.pNext = nullptr;
			entityUniformBufferDescriptorWrite.dstSet = entityDrawInfo.entityDescriptorSet;
			entityUniformBufferDescriptorWrite.dstBinding = 0;
			entityUniformBufferDescriptorWrite.dstArrayElement = 0;
			entityUniformBufferDescriptorWrite.descriptorCount = 1;
			entityUniformBufferDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			entityUniformBufferDescriptorWrite.pImageInfo = nullptr;
			entityUniformBufferDescriptorWrite.pBufferInfo = &entityUniformBufferInfo;

			vkUpdateDescriptorSets(_device, 1, &entityUniformBufferDescriptorWrite, 0, nullptr);

			EntityUniformBuffer entityUniformBuffer = {};
			entityUniformBuffer.transformationMatrix = 
				Matrix4<f32>::getTranslation(entity.position) * 
				Matrix4<f32>::getRotation(entity.yaw, entity.pitch, entity.roll) * 
				Matrix4<f32>::getScaling(entity.scaleX, entity.scaleY, entity.scaleZ);

			entityUniformBuffer.transformationMatrix.transpose();

			void* pEntityUniformData = nullptr;
			vkMapMemory(_device, entityDrawInfo.uniformBufferMemory, 0, sizeof(EntityUniformBuffer), 0, &pEntityUniformData);
			std::memcpy(pEntityUniformData, &entityUniformBuffer, sizeof(EntityUniformBuffer));
			vkUnmapMemory(_device, entityDrawInfo.uniformBufferMemory);

			_entityDrawInfos.push_back(entityDrawInfo);
		}
	}

	RenderSystemBackendVk::~RenderSystemBackendVk()
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

	VkInstance RenderSystemBackendVk::createInstance(
		const std::string& applicationName,
		const std::string& engineName,
		const std::vector<const c8*>& requiredExtensionNames,
		const VkAllocationCallbacks* pAllocator)
	{
		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pNext = nullptr;
		applicationInfo.pApplicationName = applicationName.data();
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pEngineName = engineName.data();
		applicationInfo.apiVersion = VK_MAKE_API_VERSION(1, 4, 0, 0);

		std::vector<const c8*> layerNames;
		layerNames.push_back("VK_LAYER_KHRONOS_validation");

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr;
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

	VkPhysicalDevice RenderSystemBackendVk::pickPhysicalDevice(VkInstance instance)
	{
		ui32 physicalDeviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

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

	ui32 RenderSystemBackendVk::findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		ui32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

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

	VkDevice RenderSystemBackendVk::createDevice(ui32 queueFamilyIndex, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* pAllocator)
	{
		f32 queuePriority = 1.0f;
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = nullptr;
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

		VkPhysicalDeviceDynamicRenderingFeatures deviceDynamicRenderingFeatures = {};
		deviceDynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
		deviceDynamicRenderingFeatures.pNext = nullptr;
		deviceDynamicRenderingFeatures.dynamicRendering = VK_TRUE;

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = &deviceDynamicRenderingFeatures;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
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

	VkSwapchainKHR RenderSystemBackendVk::createSwapchain(
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
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &availablePresentModeCount, nullptr);
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
		swapchainCreateInfo.pNext = nullptr;
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

	VkDescriptorPool RenderSystemBackendVk::createDescriptorPool(VkDevice device, const VkAllocationCallbacks* pAllocator)
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
		descriptorPoolCreateInfo.pNext = nullptr;
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

	VkDescriptorSet RenderSystemBackendVk::allocateDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, VkDevice device)
	{
		VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = nullptr;
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

	VkCommandPool RenderSystemBackendVk::createCommandPool(ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.pNext = nullptr;
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

	VkCommandBuffer RenderSystemBackendVk::allocateCommandBuffer(VkCommandPool commandPool, VkDevice device)
	{
		VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
		commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocateInfo.pNext = nullptr;
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

	VkBuffer RenderSystemBackendVk::createBuffer(ui32 size, VkBufferUsageFlags usage, ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = nullptr;
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

	VkImage RenderSystemBackendVk::createImage(
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
		imageCreateInfo.pNext = nullptr;
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

	VkImageView RenderSystemBackendVk::createImageView(
		VkImage image,
		VkFormat format,
		VkImageAspectFlags aspectFlags,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
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

	ui32 RenderSystemBackendVk::findMemoryTypeIndex(
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

	VkDeviceMemory RenderSystemBackendVk::allocateDeviceMemoryForBuffer(
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
		memoryAllocateInfo.pNext = nullptr;
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

	VkDeviceMemory RenderSystemBackendVk::allocateDeviceMemoryForImage(
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
		memoryAllocateInfo.pNext = nullptr;
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

	VkDescriptorSetLayout RenderSystemBackendVk::createDescriptorSetLayout(
		const std::vector<VkDescriptorSetLayoutBinding>& bindings,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.pNext = nullptr;
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

	VkPipelineLayout RenderSystemBackendVk::createPipelineLayout(
		const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
		ui32 pushConstantRangeCount,
		const VkPushConstantRange* pPushConstantRanges,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.pNext = nullptr;
		pipelineLayoutCreateInfo.flags = 0;
		pipelineLayoutCreateInfo.setLayoutCount = (ui32)descriptorSetLayouts.size();
		pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
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

	VkShaderModule RenderSystemBackendVk::createShaderModule(
		ui32 size,
		const ui32* pCode,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.pNext = nullptr;
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

	VkPipeline RenderSystemBackendVk::createComputePipeline(
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
		shaderStageCreateInfo.pSpecializationInfo = nullptr;

		VkComputePipelineCreateInfo computePipelineCreateInfo = {};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.pNext = nullptr;
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

	VkPipeline RenderSystemBackendVk::createGraphicsPipeline(
		VkShaderModule vertexShaderModule,
		VkShaderModule fragmentShaderModule,
		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo,
		VkFormat colorAttachmentFormat,
		VkFormat depthAttachmentFormat,
		ui32 viewportWidth,
		ui32 viewportHeight,
		bool enableDepthWrite,
		VkPipelineLayout pipelineLayout,
		VkDevice device,
		const VkAllocationCallbacks* pAllocator)
	{
		VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
		vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexShaderStageCreateInfo.pNext = nullptr;
		vertexShaderStageCreateInfo.flags = 0;
		vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderStageCreateInfo.module = vertexShaderModule;
		vertexShaderStageCreateInfo.pName = "main";
		vertexShaderStageCreateInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
		fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentShaderStageCreateInfo.pNext = nullptr;
		fragmentShaderStageCreateInfo.flags = 0;
		fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderStageCreateInfo.module = fragmentShaderModule;
		fragmentShaderStageCreateInfo.pName = "main";
		fragmentShaderStageCreateInfo.pSpecializationInfo = nullptr;

		VkPipelineShaderStageCreateInfo shaderStageCreateInfos[] = {
			vertexShaderStageCreateInfo,
			fragmentShaderStageCreateInfo
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
		inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCreateInfo.pNext = nullptr;
		inputAssemblyStateCreateInfo.flags = 0;
		inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (f32)viewportWidth;
		viewport.height = (f32)viewportHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = viewportWidth;
		scissor.extent.height = viewportHeight;

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
		viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCreateInfo.pNext = nullptr;
		viewportStateCreateInfo.flags = 0;
		viewportStateCreateInfo.viewportCount = 1;
		viewportStateCreateInfo.pViewports = &viewport;
		viewportStateCreateInfo.scissorCount = 1;
		viewportStateCreateInfo.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
		rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCreateInfo.pNext = nullptr;
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
		multisampleStateCreateInfo.pNext = nullptr;
		multisampleStateCreateInfo.flags = 0;
		multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampleStateCreateInfo.minSampleShading = 0.0f;
		multisampleStateCreateInfo.pSampleMask = nullptr;
		multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
		depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCreateInfo.pNext = nullptr;
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
		colorBlendStateCreateInfo.pNext = nullptr;
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


		/*
		    VkStructureType    sType;
    const void*        pNext;
    uint32_t           viewMask;
    uint32_t           colorAttachmentCount;
    const VkFormat*    pColorAttachmentFormats;
    VkFormat           depthAttachmentFormat;
    VkFormat           stencilAttachmentFormat;
		
		*/
		VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
		pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		pipelineRenderingCreateInfo.pNext = nullptr;
		pipelineRenderingCreateInfo.viewMask = 0;
		pipelineRenderingCreateInfo.colorAttachmentCount = 1;
		pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorAttachmentFormat;
		pipelineRenderingCreateInfo.depthAttachmentFormat = depthAttachmentFormat;
		pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
		graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
		graphicsPipelineCreateInfo.flags = 0;
		graphicsPipelineCreateInfo.stageCount = sizeof(shaderStageCreateInfos) / sizeof(shaderStageCreateInfos[0]);
		graphicsPipelineCreateInfo.pStages = shaderStageCreateInfos;
		graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
		graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
		graphicsPipelineCreateInfo.pTessellationState = nullptr;
		graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
		graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
		graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
		graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
		graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
		graphicsPipelineCreateInfo.pDynamicState = nullptr;
		graphicsPipelineCreateInfo.layout = pipelineLayout;
		graphicsPipelineCreateInfo.renderPass = VK_NULL_HANDLE;
		graphicsPipelineCreateInfo.subpass = 0;
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

	VkFence RenderSystemBackendVk::createFence(VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkFence fence;
		if (vkCreateFence(device, &fenceCreateInfo, pAllocator, &fence) != VK_SUCCESS)
		{
			std::cerr << "could not create fence\n";
			std::exit(EXIT_FAILURE);
		}

		return fence;
	}

	VkSemaphore RenderSystemBackendVk::createSemaphore(VkDevice device, const VkAllocationCallbacks* pAllocator)
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo = {};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		semaphoreCreateInfo.pNext = nullptr;
		semaphoreCreateInfo.flags = 0;

		VkSemaphore semaphore;
		if (vkCreateSemaphore(device, &semaphoreCreateInfo, pAllocator, &semaphore) != VK_SUCCESS)
		{
			std::cerr << "could not create semaphore\n";
			std::exit(EXIT_FAILURE);
		}

		return semaphore;
	}

	void RenderSystemBackendVk::readShader(const std::string& shaderPath, ui32* pSize, ui32** ppCode)
	{
		FILE* pFile = fopen(shaderPath.c_str(), "rb");
		if (pFile == NULL)
		{
			std::cerr << "could not open shader source " << shaderPath << "\n";
			std::exit(EXIT_FAILURE);
		}

		fseek(pFile, 0, SEEK_END);
		*pSize = ftell(pFile);
		rewind(pFile);

		*ppCode = new uint32_t[*pSize];

		fread(*ppCode, 1, *pSize, pFile);

		fclose(pFile);
	}
}