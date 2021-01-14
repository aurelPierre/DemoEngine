#pragma once

#include <vulkan/vulkan.h>

#include "ImageBuffer.h"

#include <array>

class Texture
{
public:
	enum class Format
	{
		R = 0,
		SR = R + 4,
		RG = 1,
		SRG = RG + 4,
		//RGB = 2, // TODO support rgb
		//SRGB = RGB,
		RGBA = 3,
		SRGBA = RGBA + 4
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

	uint8_t GetNumberChannels(const Format kFormat) const;
	VkFormat GetVkFormat(const Format kFormat) const;

public:
	const VkDescriptorImageInfo CreateDescriptorInfo() const;
};
