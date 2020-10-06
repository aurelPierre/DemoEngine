#pragma once

#include <vulkan/vulkan.h>

#include "Context.h"
#include "Device.h"

struct Texture
{
	VkDeviceMemory		_imageMemory	= VK_NULL_HANDLE;
	VkImage				_image			= VK_NULL_HANDLE;
	VkImageView			_imageView		= VK_NULL_HANDLE;
	VkSampler			_sampler		= VK_NULL_HANDLE;
};

Texture	CreateTexture(const Context& kContext, const LogicalDevice& kLogicalDevice,
	const Device& kDevice, const std::string kTexturePath);

void	DestroyTexture(const Context& kContext, const LogicalDevice& kLogicalDevice, Texture& texture);
