#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Device.h"
#include "Surface.h"
#include "Context.h"

struct GLFWWindowData;

struct Frame
{
	VkCommandBuffer			_commandBuffer		= VK_NULL_HANDLE;
	VkImage					_image				= VK_NULL_HANDLE;
	VkImageView				_imageView			= VK_NULL_HANDLE;
	VkFramebuffer			_framebuffer		= VK_NULL_HANDLE;

	VkFence					_fence				= VK_NULL_HANDLE;
	VkSemaphore				_presentComplete	= VK_NULL_HANDLE;
	VkSemaphore				_renderComplete		= VK_NULL_HANDLE;
};

class Swapchain
{
public:
	VkSwapchainKHR			_swapchain			= VK_NULL_HANDLE;
	VkPresentModeKHR		_presentMode		= VK_PRESENT_MODE_MAX_ENUM_KHR;
	uint32_t				_imageCount			= 0; // ie. _frames.size() ???
	uint32_t				_currentFrame		= 0;

	VkExtent2D				_size;
	VkRenderPass			_renderPass			= VK_NULL_HANDLE;

	std::vector<Frame>		_frames;

public:
	Swapchain(const Device& kDevice, const Surface& kSurface, const GLFWWindowData* windowData);
	~Swapchain();

public:
	bool	AcquireNextImage();
	void	Draw();
	void	Render();
	bool	Present();
};

void		CreateFrames(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Surface& kSurface, Swapchain& swapchain);

void	ResizeSwapchain(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Device& kDevice, const Surface& kSurface,
						const GLFWWindowData* windowData, Swapchain& swapchain);

void	DestroyFrame(const Context& kContext, const LogicalDevice& kLogicalDevice, const Frame& kFrame);
