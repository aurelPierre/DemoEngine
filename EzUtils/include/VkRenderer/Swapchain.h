#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Context.h"
#include "Device.h"
#include "Surface.h"
#include "Viewport.h"

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

struct Swapchain
{
	VkSwapchainKHR			_swapchain			= VK_NULL_HANDLE;
	VkPresentModeKHR		_presentMode		= VK_PRESENT_MODE_MAX_ENUM_KHR;
	uint32_t				_imageCount			= 0; // ie. _frames.size() ???
	uint32_t				_currentFrame		= 0;

	VkExtent2D				_size;
	VkRenderPass			_renderPass			= VK_NULL_HANDLE;

	std::vector<Frame>		_frames;
	std::vector<Viewport>	_viewports;
};

Swapchain	CreateSwapchain(const Context& kContext, const LogicalDevice& kLogicalDevice,
							const Device& kDevice, const Surface& kSurface,
							const GLFWWindowData* windowData);
void		CreateFrames(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Surface& kSurface, Swapchain& viewport);

void	Draw(const LogicalDevice& kLogicalDevice, Swapchain& swapchain);
void	Render(const LogicalDevice& kLogicalDevice, Swapchain& swapchain);
void	Present(const LogicalDevice& kLogicalDevice, Swapchain& swapchain);

void	DestroyFrame(const Context& kContext, const LogicalDevice& kLogicalDevice, const Frame& kFrame);
void	DestroySwapchain(const Context& kContext, const LogicalDevice& kLogicalDevice, const Swapchain& kSwapchain);
