#include "Surface.h"

#include "Core.h"
#include "GLFWWindowSystem.h"

Surface CreateSurface(const Context& kContext, const LogicalDevice& kLogicalDevice, const Device& kDevice,
						const GLFWWindowData* windowData)
{
	Surface surface{};

	VkResult err = glfwCreateWindowSurface(kContext._instance, windowData->_window, kContext._allocator, &surface._surface);
	check_vk_result(err);

	VkBool32 res;
	err = vkGetPhysicalDeviceSurfaceSupportKHR(kDevice._physicalDevice, kLogicalDevice._graphicsQueue._indice, surface._surface, &res);
	check_vk_result(err);
	if (res != VK_TRUE)
	{
		LOG(ez::ERROR, std::string("Error no WSI support on physical device 0"))
			exit(-1);
	}

	// Get list of supported surface formats
	uint32_t formatCount;
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(kDevice._physicalDevice, surface._surface, &formatCount, NULL);
	check_vk_result(err);
	assert(formatCount > 0);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(kDevice._physicalDevice, surface._surface, &formatCount, surfaceFormats.data());
	check_vk_result(err);

	// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
	// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
	if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
	{
		surface._colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		surface._colorSpace = surfaceFormats[0].colorSpace;
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
				surface._colorFormat = surfaceFormat.format;
				surface._colorSpace = surfaceFormat.colorSpace;
				found_B8G8R8A8_UNORM = true;
				break;
			}
		}

		// in case VK_FORMAT_B8G8R8A8_UNORM is not available
		// select the first available color format
		if (!found_B8G8R8A8_UNORM)
		{
			surface._colorFormat = surfaceFormats[0].format;
			surface._colorSpace = surfaceFormats[0].colorSpace;
		}
	}

	return surface;
}

void DestroySurface(const Context& kContext, const Surface& kSurface)
{
	vkDestroySurfaceKHR(kContext._instance, kSurface._surface, kContext._allocator);
}