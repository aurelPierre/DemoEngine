#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"
#include "ImageBuffer.h"

class Texture
{
public:
	ImageBuffer			_image;
	VkSampler			_sampler		= VK_NULL_HANDLE;

public:
	Texture(const Device& kDevice, const std::string kTexturePath);
	~Texture();
};
