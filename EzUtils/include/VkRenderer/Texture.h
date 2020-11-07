#pragma once

#include <vulkan/vulkan.h>

#include "Context.h"
#include "Device.h"

class Texture
{
public:
	VkDeviceMemory		_imageMemory	= VK_NULL_HANDLE;
	VkImage				_image			= VK_NULL_HANDLE;
	VkImageView			_imageView		= VK_NULL_HANDLE;
	VkSampler			_sampler		= VK_NULL_HANDLE;

public:
	Texture(const Device& kDevice, const std::string kTexturePath);
	~Texture();
};
