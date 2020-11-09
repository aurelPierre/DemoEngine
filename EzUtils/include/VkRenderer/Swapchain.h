#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Device.h"
#include "Surface.h"
#include "CommandBuffer.h"
#include "ImageBuffer.h"

struct GLFWWindowData;

class FrameData
{
public:
	CommandBuffer		_commandBuffer;

	VkFence				_fence				= VK_NULL_HANDLE;
	VkSemaphore			_presentComplete	= VK_NULL_HANDLE;
	VkSemaphore			_renderComplete		= VK_NULL_HANDLE;

public:
	FrameData();
	~FrameData();

	FrameData(const FrameData& kFrame) = delete;
	FrameData(FrameData&& frame);

	FrameData& operator=(const FrameData& kFrame) = delete;
	FrameData& operator=(FrameData&& frame);

private:
	void Clean();
};

class FrameImage
{
public:
	ImageBuffer			_imageBuffer;
	VkFramebuffer		_framebuffer = VK_NULL_HANDLE;

public:
	FrameImage(const VkImage kImage, const VkFormat kFormat, const VkExtent2D& kExtent, const VkRenderPass kRenderPass);

	~FrameImage();

	FrameImage(const FrameImage& kFrame) = delete;
	FrameImage(FrameImage&& frame);

	FrameImage& operator=(const FrameImage& kFrame) = delete;
	FrameImage& operator=(FrameImage&& frame);

private:
	void Clean();
};

class Swapchain
{
public:
	VkSwapchainKHR		_swapchain			= VK_NULL_HANDLE;
	VkPresentModeKHR	_presentMode		= VK_PRESENT_MODE_MAX_ENUM_KHR;
	uint32_t			_imageCount			= 0; // ie. _frames.size() ???
	uint32_t			_currentFrame		= 0;
	uint32_t			_currentImage		= 0;

	VkExtent2D			_size;
	VkRenderPass		_renderPass			= VK_NULL_HANDLE;

	std::vector<FrameData>	_framesData;
	std::vector<FrameImage> _framesImage;

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


