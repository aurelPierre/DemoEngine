#include "ImageBuffer.h"

#include "Context.h"
#include "Core.h"

ImageBuffer::ImageBuffer(const Device& kDevice, const VkFormat kFormat, const VkExtent2D& kExtent, const VkImageUsageFlags kUsage)
{
	VkImageCreateInfo image{};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = kFormat;
	image.extent.width = kExtent.width;
	image.extent.height = kExtent.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = kUsage;

	VkResult err = vkCreateImage(LogicalDevice::Instance()._device, &image, Context::Instance()._allocator, &_image);
	check_vk_result(err);

	VkMemoryAllocateInfo memAlloc{};
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(LogicalDevice::Instance()._device, _image, &memReqs);

	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlloc.allocationSize = memReqs.size;

	memAlloc.memoryTypeIndex = findMemoryType(kDevice._memoryProperties, memReqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	err = vkAllocateMemory(LogicalDevice::Instance()._device, &memAlloc, Context::Instance()._allocator, &_memory);
	check_vk_result(err);

	err = vkBindImageMemory(LogicalDevice::Instance()._device, _image, _memory, 0);
	check_vk_result(err);

	/*** Texture handling ***/
	VkImageViewCreateInfo colorAttachmentView = {};
	colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	colorAttachmentView.pNext = NULL;
	colorAttachmentView.format = kFormat;
	colorAttachmentView.components = {
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A
	};

	if (kUsage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	else
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	colorAttachmentView.subresourceRange.baseMipLevel = 0;
	colorAttachmentView.subresourceRange.levelCount = 1;
	colorAttachmentView.subresourceRange.baseArrayLayer = 0;
	colorAttachmentView.subresourceRange.layerCount = 1;
	colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorAttachmentView.flags = 0;
	colorAttachmentView.image = _image;

	err = vkCreateImageView(LogicalDevice::Instance()._device, &colorAttachmentView, Context::Instance()._allocator, &_view);
	check_vk_result(err);
}

ImageBuffer::~ImageBuffer()
{
	if(_view != VK_NULL_HANDLE)
		vkDestroyImageView(LogicalDevice::Instance()._device, _view, Context::Instance()._allocator);
	if(_image != VK_NULL_HANDLE)
		vkDestroyImage(LogicalDevice::Instance()._device, _image, Context::Instance()._allocator);
	if(_memory != VK_NULL_HANDLE)
		vkFreeMemory(LogicalDevice::Instance()._device, _memory, Context::Instance()._allocator);
}

ImageBuffer::ImageBuffer(ImageBuffer&& imageBuffer)
	: _memory { imageBuffer._memory }, _image{ imageBuffer._image }, _view { imageBuffer._view }
{
	imageBuffer._memory = VK_NULL_HANDLE;
	imageBuffer._image = VK_NULL_HANDLE;
	imageBuffer._view = VK_NULL_HANDLE;
}

ImageBuffer& ImageBuffer::operator=(ImageBuffer&& imageBuffer)
{
	if (_view != VK_NULL_HANDLE)
		vkDestroyImageView(LogicalDevice::Instance()._device, _view, Context::Instance()._allocator);
	if (_image != VK_NULL_HANDLE)
		vkDestroyImage(LogicalDevice::Instance()._device, _image, Context::Instance()._allocator);
	if (_memory != VK_NULL_HANDLE)
		vkFreeMemory(LogicalDevice::Instance()._device, _memory, Context::Instance()._allocator);

	_memory = imageBuffer._memory;
	_image = imageBuffer._image;
	_view = imageBuffer._view;

	imageBuffer._memory = VK_NULL_HANDLE;
	imageBuffer._image = VK_NULL_HANDLE;
	imageBuffer._view = VK_NULL_HANDLE;

	return *this;
}