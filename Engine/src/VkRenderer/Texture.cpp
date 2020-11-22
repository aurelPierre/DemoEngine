#include "VkRenderer/Texture.h"

#include "VkRenderer/Core.h"
#include "VkRenderer/Context.h"

#include "VkRenderer/Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(const std::string kTexturePath, const bool kUseSRGB)
{
	ASSERT(!kTexturePath.empty(), "kTexturePath is empty")

	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels = stbi_load(kTexturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	ASSERT(pixels, "failed to load texture image " + kTexturePath + " !")

	Buffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer.Map(pixels, static_cast<size_t>(imageSize));
	
	stbi_image_free(pixels);

	VkFormat imageFormat = kUseSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM; // TODO: add R8G8B8 support
	if (texChannels == 4)
		imageFormat = kUseSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	else if(texChannels == 2)
		imageFormat = kUseSRGB ? VK_FORMAT_R8G8_SRGB : VK_FORMAT_R8G8_UNORM;
	else if (texChannels == 1)
		imageFormat = kUseSRGB ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;

	ImageBuffer image(imageFormat, { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) },
						VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	_image = std::move(image);

	_image.TransitionLayout(LogicalDevice::Instance()._transferQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	_image.CopyBuffer(LogicalDevice::Instance()._transferQueue, stagingBuffer);
	_image.TransitionLayout(LogicalDevice::Instance()._graphicsQueue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

	VkResult err = vkCreateSampler(LogicalDevice::Instance()._device, &samplerInfo, Context::Instance()._allocator, &_sampler);
	VK_ASSERT(err, "failed to create texture sampler!")
}

Texture::~Texture()
{
	vkDestroySampler(LogicalDevice::Instance()._device, _sampler, Context::Instance()._allocator);
}

const VkDescriptorImageInfo Texture::CreateDescriptorInfo() const
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = _image._view;
	imageInfo.sampler = _sampler;

	return imageInfo;
}