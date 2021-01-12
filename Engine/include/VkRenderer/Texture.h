#pragma once

#include <vulkan/vulkan.h>

#include "ImageBuffer.h"

class Texture
{
public:
	// TODO support more format
	// List of format supported at the moment
	enum class Format
	{
		R,
		SR,
		RGBA,
		SRGBA
	};

public:
	ImageBuffer			_image;
	VkSampler			_sampler		= VK_NULL_HANDLE;

public:
	Texture(const std::string kTexturePath, const Format kFormat = Format::RGBA);
	~Texture();

public:
	const VkDescriptorImageInfo CreateDescriptorInfo() const;
};
