#pragma once

#include <vulkan/vulkan.h>

#include "ImageBuffer.h"

#include <array>

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
	Texture(const std::array<std::string, 6> kCubemapPath, const Format kFormat = Format::RGBA);

	~Texture();

private:
	void CreateSampler();

public:
	const VkDescriptorImageInfo CreateDescriptorInfo() const;
};
