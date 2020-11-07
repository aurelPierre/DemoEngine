#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"

class ImageBuffer
{
public:
	VkDeviceMemory	_memory	= VK_NULL_HANDLE;
	VkImage			_image	= VK_NULL_HANDLE;
	VkImageView		_view	= VK_NULL_HANDLE;

public:
	ImageBuffer() = default;
	ImageBuffer(const Device& kDevice, const VkFormat kFormat, const VkExtent2D& kExtent, const VkImageUsageFlags kUsage);

	~ImageBuffer();

	ImageBuffer(const ImageBuffer& kImageBuffer) = delete;
	ImageBuffer(ImageBuffer&& imageBuffer);

	ImageBuffer& operator=(const ImageBuffer& kImageBuffer) = delete;
	ImageBuffer& operator=(ImageBuffer&& imageBuffer);
};