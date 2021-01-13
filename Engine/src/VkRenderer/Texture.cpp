#include "VkRenderer/Texture.h"

#include "Core.h"
#include "VkRenderer/Context.h"

#include "VkRenderer/Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(const std::string kTexturePath, const Format kFormat)
{
	ASSERT(!kTexturePath.empty(), "kTexturePath is empty")

	int reqComp = STBI_rgb_alpha;
	if (kFormat == Format::R || kFormat == Format::SR)
		reqComp = STBI_grey;

	int texWidth = 0, texHeight = 0, texChannels = 0;
	stbi_uc* pixels = stbi_load(kTexturePath.c_str(), &texWidth, &texHeight, &texChannels, reqComp);
	VkDeviceSize imageSize = texWidth * texHeight * (reqComp == STBI_grey ? 1 : 4);

	ASSERT(pixels, "failed to load texture image " + kTexturePath + " !")

	Buffer stagingBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	stagingBuffer.Map(pixels, static_cast<size_t>(imageSize));
	
	stbi_image_free(pixels);

	VkFormat imageFormat = kFormat == Format::SRGBA ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	if (reqComp == STBI_grey)
		imageFormat = kFormat == Format::SR ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;

	ImageBuffer image(imageFormat, { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) },
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	_image = std::move(image);

	_image.TransitionLayout(LogicalDevice::Instance()._transferQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	_image.CopyBuffer(LogicalDevice::Instance()._transferQueue, stagingBuffer);
	_image.TransitionLayout(LogicalDevice::Instance()._graphicsQueue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	CreateSampler();
}

Texture::Texture(const std::array<std::string, 6> kCubemapPath, const Format kFormat)
{
	int reqComp = STBI_rgb_alpha;
	if (kFormat == Format::R || kFormat == Format::SR)
		reqComp = STBI_grey;

	VkDeviceSize cubemapSize = 0;
	stbi_uc* pixels[6]{};

	int cubeWidth = 0, cubeHeight = 0;
	for (int i = 0; i < 6; ++i)
	{
		ASSERT(!kCubemapPath[i].empty(), "kTexturePath is empty")
		int texWidth = 0, texHeight = 0, texChannels = 0;
		pixels[i] = stbi_load(kCubemapPath[i].c_str(), &texWidth, &texHeight, &texChannels, reqComp);

		ASSERT(pixels[i], "failed to load texture image " + kCubemapPath[i] + " !")
		cubemapSize += texWidth * texHeight * (reqComp == STBI_grey ? 1 : 4);

		ASSERT((texWidth == cubeWidth || cubeWidth == 0) || (texHeight == cubeHeight || cubeHeight == 0),
			"texture " + kCubemapPath[i] + " is not the same size as previous cubemap texture")

		cubeWidth = texWidth;
		cubeHeight = texHeight;
	}

	Buffer stagingBuffer(cubemapSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	for (int i = 0; i < 6; ++i)
	{
		stagingBuffer.Map(pixels[i], static_cast<size_t>(cubemapSize / 6), static_cast<size_t>((cubemapSize / 6) * i));
		stbi_image_free(pixels[i]);
	}

	VkFormat imageFormat = kFormat == Format::SRGBA ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
	if (reqComp == STBI_grey)
		imageFormat = kFormat == Format::SR ? VK_FORMAT_R8_SRGB : VK_FORMAT_R8_UNORM;

	ImageBuffer image(imageFormat, { static_cast<uint32_t>(cubeWidth), static_cast<uint32_t>(cubeHeight) },
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, true);
	_image = std::move(image);

	_image.TransitionLayout(LogicalDevice::Instance()._transferQueue, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	_image.CopyBuffer(LogicalDevice::Instance()._transferQueue, stagingBuffer);
	_image.TransitionLayout(LogicalDevice::Instance()._graphicsQueue, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	CreateSampler();
}


Texture::~Texture()
{
	vkDestroySampler(LogicalDevice::Instance()._device, _sampler, Context::Instance()._allocator);
}

void Texture::CreateSampler()
{
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
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkResult err = vkCreateSampler(LogicalDevice::Instance()._device, &samplerInfo, Context::Instance()._allocator, &_sampler);
	VK_ASSERT(err, "failed to create texture sampler!")
}

const VkDescriptorImageInfo Texture::CreateDescriptorInfo() const
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = _image._view;
	imageInfo.sampler = _sampler;

	return imageInfo;
}