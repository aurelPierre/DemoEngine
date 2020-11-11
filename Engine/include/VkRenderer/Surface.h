#pragma once

#include <vulkan/vulkan.h>

struct GLFWWindowData;

class Surface
{
public:
	VkSurfaceKHR		_surface		= VK_NULL_HANDLE;
	VkFormat			_colorFormat	= VK_FORMAT_MAX_ENUM;
	VkColorSpaceKHR		_colorSpace		= VK_COLOR_SPACE_MAX_ENUM_KHR;

public:
	Surface(const GLFWWindowData* windowData);
	~Surface();
};
