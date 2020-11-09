#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"
#include "Buffer.h"

class ImageBuffer
{
public:
	VkExtent2D		_size;
	VkDeviceMemory	_memory	= VK_NULL_HANDLE;
	VkImage			_image	= VK_NULL_HANDLE;
	VkImageView		_view	= VK_NULL_HANDLE;

public:
	ImageBuffer() = default;
	ImageBuffer(const VkImage kImage, const VkFormat kFormat, const VkExtent2D& kExtent, const VkImageUsageFlags kUsage);
	ImageBuffer(const Device& kDevice, const VkFormat kFormat, const VkExtent2D& kExtent, const VkImageUsageFlags kUsage);

	~ImageBuffer();

	ImageBuffer(const ImageBuffer& kImageBuffer) = delete;
	ImageBuffer(ImageBuffer&& imageBuffer);

	ImageBuffer& operator=(const ImageBuffer& kImageBuffer) = delete;
	ImageBuffer& operator=(ImageBuffer&& imageBuffer);

private:
	void CreateView(const VkFormat kFormat, const VkImageUsageFlags kUsage);
	void Clean();

public:
	void TransitionLayout(const Queue& kQueue, const VkImageLayout kOldLayout, const VkImageLayout kNewLayout) const;
	void CopyBuffer(const Queue& kQueue, const Buffer& kBuffer) const;
};