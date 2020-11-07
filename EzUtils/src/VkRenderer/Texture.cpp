#include "Texture.h"

#include "Context.h"
#include "Core.h"

#include "Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(const Device& kDevice, const std::string kTexturePath)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(kTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	Buffer stagingBuffer(kDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer.Map(pixels, static_cast<size_t>(imageSize));
	
	stbi_image_free(pixels);

	ImageBuffer image(kDevice, VK_FORMAT_R8G8B8A8_UNORM, { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) },
						VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	_image = std::move(image);

	transitionImageLayout(LogicalDevice::Instance()._device, LogicalDevice::Instance()._graphicsQueue._queue,
		LogicalDevice::Instance()._graphicsQueue._commandPool, _image._image,
		VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(LogicalDevice::Instance()._device, LogicalDevice::Instance()._graphicsQueue._queue,
		LogicalDevice::Instance()._graphicsQueue._commandPool, stagingBuffer._buffer, 
		_image._image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(LogicalDevice::Instance()._device, LogicalDevice::Instance()._graphicsQueue._queue,
			LogicalDevice::Instance()._graphicsQueue._commandPool, _image._image, VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 16.0f;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(LogicalDevice::Instance()._device, &samplerInfo, Context::Instance()._allocator, &_sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}
}

Texture::~Texture()
{
	vkDestroySampler(LogicalDevice::Instance()._device, _sampler, Context::Instance()._allocator);
}