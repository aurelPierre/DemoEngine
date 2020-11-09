#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Device.h"
#include "Surface.h"
#include "CommandBuffer.h"

struct GLFWWindowData;

class Frame
{
public:
	CommandBuffer		_commandBuffer;
	VkImage				_image				= VK_NULL_HANDLE;
	VkImageView			_imageView			= VK_NULL_HANDLE;
	VkFramebuffer		_framebuffer		= VK_NULL_HANDLE;

	VkFence				_fence				= VK_NULL_HANDLE;
	VkSemaphore			_presentComplete	= VK_NULL_HANDLE;
	VkSemaphore			_renderComplete		= VK_NULL_HANDLE;

public:
	Frame(const VkImage kImage, const VkFormat kFormat, const VkExtent2D& kExtent, const VkRenderPass kRenderPass);

	~Frame();

	Frame(const Frame& kFrame) = delete;
	Frame(Frame&& frame);

	Frame& operator=(const Frame& kFrame) = delete;
	Frame& operator=(Frame&& frame);
};

class Swapchain
{
public:
	VkSwapchainKHR		_swapchain			= VK_NULL_HANDLE;
	VkPresentModeKHR	_presentMode		= VK_PRESENT_MODE_MAX_ENUM_KHR;
	uint32_t			_imageCount			= 0; // ie. _frames.size() ???
	uint32_t			_currentFrame		= 0;

	VkExtent2D			_size;
	VkRenderPass		_renderPass			= VK_NULL_HANDLE;

	std::vector<Frame>	_frames;

public:
	Swapchain(const Device& kDevice, const Surface& kSurface, const GLFWWindowData* windowData);

	~Swapchain();

	Swapchain(const Swapchain& kSwapchain) = delete;
	Swapchain& operator=(const Swapchain& kSwapchain) = delete;

private:
	void Init(const Device& kDevice, const Surface& kSurface, const GLFWWindowData* windowData);
	void Clean();

public:
	void	Resize(const Device& kDevice, const Surface& kSurface, const GLFWWindowData* windowData);
	bool	AcquireNextImage();
	void	Draw();
	void	Render();
	bool	Present();
};


