#pragma once

#include "types.h"
#include "window.h"
#include "camera.h"
#include "entity.h"
#include "maths.h"

#include "volk.h"

#include <string>
#include <vector>

namespace Visor
{
	class RendererBackendVk
	{
	public:
		void render(const Camera& camera);

		static void start(Window& window, const std::vector<Entity>& entities);
		static void terminate();
		static RendererBackendVk& getInstance();

	private:
		struct EntityDrawInfo
		{
			VkDescriptorSetLayout entityDescriptorSetLayout;
			VkDescriptorSet entityDescriptorSet;
			VkPipelineLayout graphicsPipelineLayout;
			VkPipeline graphicsPipeline;
			ui32 vertexCount;
			VkBuffer vertexBuffer;
			VkDeviceMemory vertexBufferMemory;
			VkBuffer uniformBuffer;
			VkDeviceMemory uniformBufferMemory;
		};

		struct GlobalUniformBuffer
		{
			Matrix4<f32> viewProjectionMatrix;
		};

		struct EntityUniformBuffer
		{
			Matrix4<f32> transformationMatrix;
		};

	private:
		RendererBackendVk(Window& window, const std::vector<Entity>& entities);
		~RendererBackendVk();

		VkInstance createInstance(
			const std::string& applicationName,
			const std::string& engineName,
			const std::vector<const c8*>& requiredExtensionNames,
			const VkAllocationCallbacks* pAllocator);
		VkPhysicalDevice pickPhysicalDevice(VkInstance instance);
		ui32 findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		VkDevice createDevice(ui32 queueFamilyIndex, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* pAllocator);
		VkSwapchainKHR createSwapchain(
			VkPhysicalDevice physicalDevice,
			VkFormat format,
			ui32 width,
			ui32 height,
			VkDevice device,
			VkSurfaceKHR surface,
			ui32 queueFamilyIndex,
			const VkAllocationCallbacks* pAllocator);
		VkDescriptorPool createDescriptorPool(VkDevice device, const VkAllocationCallbacks* pAllocator);
		VkDescriptorSet allocateDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, VkDevice device);
		VkCommandPool createCommandPool(ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator);
		VkCommandBuffer allocateCommandBuffer(VkCommandPool commandPool, VkDevice device);
		VkBuffer createBuffer(ui32 size, VkBufferUsageFlags usage, ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator);
		VkImage createImage(
			VkFormat format,
			ui32 width,
			ui32 height,
			VkImageUsageFlags usage,
			ui32 queueFamilyIndex,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		VkImageView createImageView(
			VkImage image,
			VkFormat format,
			VkImageAspectFlags aspectFlags,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		ui32 findMemoryTypeIndex(
			VkPhysicalDevice physicalDevice,
			ui32 compatibleMemoryTypeBits,
			VkMemoryPropertyFlags memoryPropertyFlags);
		VkDeviceMemory allocateDeviceMemoryForBuffer(
			VkDevice device,
			VkBuffer buffer,
			VkPhysicalDevice physicalDevice,
			VkMemoryPropertyFlags memoryPropertyFlags,
			const VkAllocationCallbacks* pAllocator);
		VkDeviceMemory allocateDeviceMemoryForImage(
			VkDevice device,
			VkImage image,
			VkPhysicalDevice physicalDevice,
			VkMemoryPropertyFlags memoryPropertyFlags,
			const VkAllocationCallbacks* pAllocator);
		VkDescriptorSetLayout createDescriptorSetLayout(
			const std::vector<VkDescriptorSetLayoutBinding>& bindings,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		VkPipelineLayout createPipelineLayout(
			const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
			ui32 pushConstantRangeCount,
			const VkPushConstantRange* pPushConstantRanges,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		VkShaderModule createShaderModule(
			ui32 size,
			const ui32* pCode,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		VkPipeline createComputePipeline(
			VkShaderModule shaderModule,
			VkPipelineLayout pipelineLayout,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		VkPipeline createGraphicsPipeline(
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
			const VkAllocationCallbacks* pAllocator);
		VkFence createFence(VkDevice device, const VkAllocationCallbacks* pAllocator);
		VkSemaphore createSemaphore(VkDevice device, const VkAllocationCallbacks* pAllocator);

		void readShader(const std::string& shaderPath, ui32* pSize, ui32** ppCode);

	private:
		VkAllocationCallbacks* _pAllocator;
		VkInstance _instance;
		VkSurfaceKHR _surface;
		VkPhysicalDevice _physicalDevice;
		ui32 _queueFamilyIndex;
		VkDevice _device;
		VkQueue _queue;
		VkRect2D _renderArea;
		VkSwapchainKHR _swapchain;
		VkFormat _swapchainFormat;
		std::vector<VkImage> _swapchainImages;
		std::vector<VkImageView> _swapchainImageViews;
		VkSemaphore _imageAvailableSemaphore;
		std::vector<VkSemaphore> _imageRenderedSemaphores;
		VkFence _commandBufferExecutedFence;
		VkCommandPool _commandPool;
		VkDescriptorPool _descriptorPool;
		VkCommandBuffer _commandBuffer;

		// draw specific objects
		VkImage _depthImage;
		VkImageView _depthImageView;
		VkDeviceMemory _depthImageMemory;
		VkDescriptorSet _globalDescriptorSet;
		VkDescriptorSetLayout _globalDescriptorSetLayout;
		VkBuffer _globalUniformBuffer;
		VkDeviceMemory _globalUniformBufferMemory;
		std::vector<EntityDrawInfo> _entityDrawInfos;
	};
}