#pragma once

#include <vulkan/vulkan.h>

#include "ImageBuffer.h"

class Texture
{
public:
	ImageBuffer			_image;
	VkSampler			_sampler		= VK_NULL_HANDLE;

public:
	Texture(const std::string kTexturePath, const VkFormat kFormat);
	~Texture();

public:
	const VkDescriptorImageInfo CreateDescriptorInfo() const;
};
