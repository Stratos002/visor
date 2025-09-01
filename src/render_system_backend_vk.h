#pragma once

#include "types.h"
#include "camera.h"
#include "entity.h"
#include "maths.h"

#include "volk.h"

#include <string>
#include <vector>

namespace Visor
{
	class RenderSystemBackendVk
	{
	public:
		void render(const Camera& camera, const std::vector<Entity>& entities);

		static void start();
		static void terminate();
		static RenderSystemBackendVk& getInstance();

	private:
		struct EntityDrawInfo
		{
			VkDescriptorSetLayout entityDescriptorSetLayout;
			VkDescriptorSet entityDescriptorSet;
			VkPipelineLayout graphicsPipelineLayout;
			VkPipeline graphicsPipeline;
			VkBuffer vertexBuffer;
			VkDeviceMemory vertexBufferMemory;
			ui32 indexCount;
			VkBuffer indexBuffer;
			VkDeviceMemory indexBufferMemory;
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
		RenderSystemBackendVk();
		~RenderSystemBackendVk();

		void updateGlobalUniformBuffer(const Camera& camera);
		void updateEntityDrawInfos(const std::vector<Entity>& entities);
		void createEntityDrawInfos(const std::vector<Entity>& entities);
		void destroyEntityDrawInfos();
		void destroyFrameObjects();

		static VkInstance createInstance(
			const std::string& applicationName,
			const std::string& engineName,
			const std::vector<const c8*>& requiredExtensionNames,
			const VkAllocationCallbacks* pAllocator);
		static VkPhysicalDevice pickPhysicalDevice(VkInstance instance);
		static ui32 findQueueFamilyIndex(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
		static VkDevice createDevice(ui32 queueFamilyIndex, VkPhysicalDevice physicalDevice, const VkAllocationCallbacks* pAllocator);
		static VkSwapchainKHR createSwapchain(
			VkPhysicalDevice physicalDevice,
			VkFormat format,
			ui32 width,
			ui32 height,
			VkDevice device,
			VkSurfaceKHR surface,
			ui32 queueFamilyIndex,
			const VkAllocationCallbacks* pAllocator);
		static VkDescriptorPool createDescriptorPool(VkDevice device, const VkAllocationCallbacks* pAllocator);
		static VkDescriptorSet allocateDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, VkDevice device);
		static VkCommandPool createCommandPool(ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator);
		static VkCommandBuffer allocateCommandBuffer(VkCommandPool commandPool, VkDevice device);
		static VkBuffer createBuffer(ui32 size, VkBufferUsageFlags usage, ui32 queueFamilyIndex, VkDevice device, const VkAllocationCallbacks* pAllocator);
		static VkImage createImage(
			VkFormat format,
			ui32 width,
			ui32 height,
			VkImageUsageFlags usage,
			ui32 queueFamilyIndex,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		static VkImageView createImageView(
			VkImage image,
			VkFormat format,
			VkImageAspectFlags aspectFlags,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		static ui32 findMemoryTypeIndex(
			VkPhysicalDevice physicalDevice,
			ui32 compatibleMemoryTypeBits,
			VkMemoryPropertyFlags memoryPropertyFlags);
		static VkDeviceMemory allocateDeviceMemoryForBuffer(
			VkDevice device,
			VkBuffer buffer,
			VkPhysicalDevice physicalDevice,
			VkMemoryPropertyFlags memoryPropertyFlags,
			const VkAllocationCallbacks* pAllocator);
		static VkDeviceMemory allocateDeviceMemoryForImage(
			VkDevice device,
			VkImage image,
			VkPhysicalDevice physicalDevice,
			VkMemoryPropertyFlags memoryPropertyFlags,
			const VkAllocationCallbacks* pAllocator);
		static VkDescriptorSetLayout createDescriptorSetLayout(
			const std::vector<VkDescriptorSetLayoutBinding>& bindings,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		static VkPipelineLayout createPipelineLayout(
			const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts,
			ui32 pushConstantRangeCount,
			const VkPushConstantRange* pPushConstantRanges,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		static VkShaderModule createShaderModule(
			ui32 size,
			const ui32* pCode,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		static VkPipeline createComputePipeline(
			VkShaderModule shaderModule,
			VkPipelineLayout pipelineLayout,
			VkDevice device,
			const VkAllocationCallbacks* pAllocator);
		static VkPipeline createGraphicsPipeline(
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
		static VkFence createFence(VkDevice device, const VkAllocationCallbacks* pAllocator);
		static VkSemaphore createSemaphore(VkDevice device, const VkAllocationCallbacks* pAllocator);		
		static void readShader(const std::string& shaderPath, ui32* pSize, ui32** ppCode);

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