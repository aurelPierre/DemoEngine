#pragma once

#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
	#define IMGUI_VULKAN_DEBUG_REPORT
#endif

struct GLFWWindowData;

/**************************************************************************************
* Vulkan setup system order ( data from https://vulkan-tutorial.com/ ):
*--------------------------------------------------------------------------------------
*	- CreateInstance				***		CONTEXT		***			--> DONE
*	- SetupDebug					***		CONTEXT		***			--> DONE
*	- PickPhysicalDevice			***		DEVICE		***			--> 
*	- CreateLogicalDevice			***		DEVICE		***			--> 
*	- CreateSurface					***		WINDOW		***			--> 
*	- CreateSwapchain				***		WINDOW		***			--> 
*	- CreateImageViews				***		FRAME		***			--> 
*	- CreateRenderPass				***		WINDOW		***			--> 
*	- CreateGraphicsPipeline		***		WINDOW		***			--> 
*	- CreateFramebuffers			***		FRAME		***			--> 
*	- CreateCommandPool				***		FRAME		***			--> 
*	- CreateCommandBuffers			***		FRAME		***			--> 
*	- CreateSemaphores				***		FRAME		***			--> 
***************************************************************************************/

struct VulkanContext
{
	VkInstance								_instance				= VK_NULL_HANDLE;
	VkAllocationCallbacks*					_allocator				= NULL;
	VkDebugUtilsMessengerEXT				_debugMessenger			= VK_NULL_HANDLE;
};

struct VulkanDevice
{
	VkPhysicalDevice						_physicalDevice			= VK_NULL_HANDLE;
	VkDevice								_device					= VK_NULL_HANDLE;
	VkPhysicalDeviceProperties				_properties;
	VkPhysicalDeviceFeatures				_features;
	VkPhysicalDeviceFeatures				_enabledFeatures;
	VkPhysicalDeviceMemoryProperties		_memoryProperties;
	std::vector<VkQueueFamilyProperties>	_queueFamilyProperties;
	std::vector<std::string>				_supportedExtensions;

	struct QueueFamilyIndices // why ?
	{
		uint32_t	_graphics	= 0;
		uint32_t	_compute	= 0;
		uint32_t	_transfer	= 0;
	};

	QueueFamilyIndices						_queueFamilyIndices;
	VkQueue									_graphicsQueue			= VK_NULL_HANDLE;
	VkQueue									_computeQueue			= VK_NULL_HANDLE;
	VkQueue									_transferQueue			= VK_NULL_HANDLE;

	VkDescriptorPool						_descriptorPool			= VK_NULL_HANDLE;
};

struct VulkanFrame
{
	VkCommandBuffer							_commandBuffer			= VK_NULL_HANDLE;
	VkImage									_image					= VK_NULL_HANDLE;
	VkImageView								_imageView				= VK_NULL_HANDLE;
	VkFramebuffer							_framebuffer			= VK_NULL_HANDLE;
	VkSampler								_sampler				= VK_NULL_HANDLE;

	VkFence									_fence					= VK_NULL_HANDLE;
	VkSemaphore								_presentComplete		= VK_NULL_HANDLE;
	VkSemaphore								_renderComplete			= VK_NULL_HANDLE;
};

struct VulkanTexture
{
	VkDeviceMemory							_imageMemory			= VK_NULL_HANDLE;
	VkImage									_image					= VK_NULL_HANDLE;
	VkImageView								_imageView				= VK_NULL_HANDLE;
	VkSampler								_sampler				= VK_NULL_HANDLE;
	VkDescriptorSet							_set					= VK_NULL_HANDLE;
};


struct SceneRendering
{
	VkRenderPass							_renderPass				= VK_NULL_HANDLE;
	VkDeviceMemory							_imageMemory			= VK_NULL_HANDLE;
	VkImage									_image					= VK_NULL_HANDLE;
	VkImageView								_imageView				= VK_NULL_HANDLE;
	VkSampler								_sampler				= VK_NULL_HANDLE;
	VkFramebuffer							_framebuffer			= VK_NULL_HANDLE;
	VkDescriptorSet							_set					= VK_NULL_HANDLE;
	VkCommandBuffer							_commandBuffer			= VK_NULL_HANDLE;
};

struct VulkanWindow
{
	// TODO abstract ??
	/* gflw window data */
	const GLFWWindowData*					_windowData				= VK_NULL_HANDLE;

	/* Window data */
	VkSurfaceKHR							_surface				= VK_NULL_HANDLE;
	VkFormat								_colorFormat			= VK_FORMAT_MAX_ENUM;
	VkColorSpaceKHR							_colorSpace				= VK_COLOR_SPACE_MAX_ENUM_KHR;

	/* Swapchain */
	VkSwapchainKHR							_swapchain				= VK_NULL_HANDLE;
	VkPresentModeKHR						_presentMode			= VK_PRESENT_MODE_MAX_ENUM_KHR;
	uint32_t								_imageCount				= 0;
	uint32_t								_currentFrame			= 0;
	std::vector<VulkanFrame>				_frames;

	// TODO should not be here
	VkCommandPool							_commandPool			= VK_NULL_HANDLE;

	VkRenderPass							_renderPass				= VK_NULL_HANDLE;
	VkPipelineCache							_pipelineCache			= VK_NULL_HANDLE;

	// DEMO
	VulkanTexture _texture;
	SceneRendering _scene;
};


class VulkanRendererSystem
{
	VulkanContext		_contextData;
	VulkanDevice		_deviceData;
	VulkanWindow		_windowData;

	//VulkanObject		_objects;

private:
	// Context
	void				CreateInstance();
	void				SetupDebug();

	// Device
	int					RateDeviceSuitability(VkPhysicalDevice device);
	void				PickPhysicalDevice();

	void				CreateLogicalDevice();

	// Window
	void				CreateSurface(const GLFWWindowData* windowData);
	void				CreateSwapchain(const GLFWWindowData* windowData);

	void				CreateRenderPass();
	void				CreateGraphicsPipeline(); // TODO might change

	// Frame
	void				CreateImageViews();
	void				CreateFramebuffers();

	// SHADERS
	VkShaderModule					loadShader(std::string path);
	VkPipelineShaderStageCreateInfo createShader(VkShaderModule shaderModule, VkShaderStageFlagBits flags);

public:
	VulkanContext*		CreateVulkanContext();
	void				DeleteVulkanContext();

	VulkanDevice*		CreateVulkanDevice();
	void				DeleteVulkanDevice();

	VulkanWindow*		CreateVulkanWindow(const GLFWWindowData* windowData);
	void				DeleteVulkanWindow();
	void				ResetVulkanWindow();

	void				Clear();

	VkCommandBuffer		PrepareDraw();
	void				SubmitDraw();

	void				Render();
	void				Present();

	/******************************************************** TEMP ***********************************************/
	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	/************************************************************************************************************/

	void				CreateSceneRendering();
	void				LoadTexture();
	void				Debug(); // Imgui draw
};