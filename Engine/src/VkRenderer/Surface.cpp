#include "VkRenderer/Surface.h"

#include "Core.h"
#include "VkRenderer/Context.h"
#include "VkRenderer/Device.h"

#include "GLFWWindowSystem.h"

Surface::Surface(const GLFWWindowData* windowData)
{
	ASSERT(windowData != nullptr, "windowData is nullptr")

	VkResult err = glfwCreateWindowSurface(Context::Instance()._instance, windowData->_window, Context::Instance()._allocator, &_surface);
	VK_ASSERT(err, "error when creating window surface");

	VkBool32 res;
	err = vkGetPhysicalDeviceSurfaceSupportKHR(LogicalDevice::Instance()._physicalDevice->_physicalDevice, LogicalDevice::Instance()._graphicsQueue._indice, _surface, &res);
	VK_ASSERT(err, "error when getting physical device surface support KHR");
	if (res != VK_TRUE)
	{
		LOG(ez::ERROR, std::string("Error no WSI support on physical device 0"))
			exit(-1);
	}

	// Get list of supported surface formats
	uint32_t formatCount;
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(LogicalDevice::Instance()._physicalDevice->_physicalDevice, _surface, &formatCount, NULL);
	VK_ASSERT(err, "error when getting physical device surface formats KHR");
	assert(formatCount > 0);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(LogicalDevice::Instance()._physicalDevice->_physicalDevice, _surface, &formatCount, surfaceFormats.data());
	VK_ASSERT(err, "error when getting physical device surface formats KHR");

	// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
	// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
	if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
	{
		_colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		_colorSpace = surfaceFormats[0].colorSpace;
	}
	else
	{
		// iterate over the list of available surface format and
		// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
		bool found_B8G8R8A8_UNORM = false;
		for (VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				_colorFormat = surfaceFormat.format;
				_colorSpace = surfaceFormat.colorSpace;
				found_B8G8R8A8_UNORM = true;
				break;
			}
		}

		// in case VK_FORMAT_B8G8R8A8_UNORM is not available
		// select the first available color format
		if (!found_B8G8R8A8_UNORM)
		{
			_colorFormat = surfaceFormats[0].format;
			_colorSpace = surfaceFormats[0].colorSpace;
		}
	}
}

Surface::~Surface()
{
	vkDestroySurfaceKHR(Context::Instance()._instance, _surface, Context::Instance()._allocator);
}