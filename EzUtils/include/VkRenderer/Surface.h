#pragma once

#include <vulkan/vulkan.h>

#include "Context.h"
#include "Device.h"

struct GLFWWindowData;

struct Surface
{
	VkSurfaceKHR		_surface		= VK_NULL_HANDLE;
	VkFormat			_colorFormat	= VK_FORMAT_MAX_ENUM;
	VkColorSpaceKHR		_colorSpace		= VK_COLOR_SPACE_MAX_ENUM_KHR;
};

Surface CreateSurface(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Device& kDevice, const GLFWWindowData* windowData);

void DestroySurface(const Context& kContext, const Surface& kSurface);