#include "VkRenderer/ImageBuffer.h"

#include "Core.h"
#include "VkRenderer/Context.h"
#include "VkRenderer/CommandBuffer.h"

ImageBuffer::ImageBuffer(const VkImage kImage, const VkFormat kFormat, const VkExtent2D& kExtent, const VkImageUsageFlags kUsage)
	: _size{ kExtent }, _image{ kImage }
{
	ASSERT(kImage != nullptr, "kImage is nullptr")
	ASSERT(kExtent.width != 0u && kExtent.height != 0, "kExtent.width is 0 or kExtent.height is 0")

	CreateView(kFormat, kUsage);
}

ImageBuffer::ImageBuffer(const VkFormat kFormat, const VkExtent2D& kExtent, const VkImageUsageFlags kUsage)
	: _size{ kExtent }
{
	ASSERT(kExtent.width != 0u && kExtent.height != 0, "kExtent.width is 0 or kExtent.height is 0")

	VkImageCreateInfo image{};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = kFormat;
	image.extent.width = _size.width;
	image.extent.height = _size.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = kUsage;

	VkResult err = vkCreateImage(LogicalDevice::Instance()._device, &image, Context::Instance()._allocator, &_image);
	VK_ASSERT(err, "error when creating image");

	VkMemoryAllocateInfo memAlloc{};
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(LogicalDevice::Instance()._device, _image, &memReqs);

	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAlloc.allocationSize = memReqs.size;

	memAlloc.memoryTypeIndex = LogicalDevice::Instance()._physicalDevice->
									FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	err = vkAllocateMemory(LogicalDevice::Instance()._device, &memAlloc, Context::Instance()._allocator, &_memory);
	VK_ASSERT(err, "error when allocating memory");

	err = vkBindImageMemory(LogicalDevice::Instance()._device, _image, _memory, 0);
	VK_ASSERT(err, "error when binding image memory");

	CreateView(kFormat, kUsage);
}

ImageBuffer::~ImageBuffer()
{
	Clean();
}

ImageBuffer::ImageBuffer(ImageBuffer&& imageBuffer)
	: _size{ imageBuffer._size }, _memory { imageBuffer._memory }, _image{ imageBuffer._image }, _view{ imageBuffer._view }
{
	imageBuffer._memory = VK_NULL_HANDLE;
	imageBuffer._image = VK_NULL_HANDLE;
	imageBuffer._view = VK_NULL_HANDLE;
}

ImageBuffer& ImageBuffer::operator=(ImageBuffer&& imageBuffer)
{
	Clean();

	_size = imageBuffer._size;
	_memory = imageBuffer._memory;
	_image = imageBuffer._image;
	_view = imageBuffer._view;

	imageBuffer._memory = VK_NULL_HANDLE;
	imageBuffer._image = VK_NULL_HANDLE;
	imageBuffer._view = VK_NULL_HANDLE;

	return *this;
}

void ImageBuffer::CreateView(const VkFormat kFormat, const VkImageUsageFlags kUsage)
{
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

	VkResult err = vkCreateImageView(LogicalDevice::Instance()._device, &colorAttachmentView, Context::Instance()._allocator, &_view);
	VK_ASSERT(err, "error when creating image view");
}

void ImageBuffer::Clean()
{
	if (_view != VK_NULL_HANDLE)
		vkDestroyImageView(LogicalDevice::Instance()._device, _view, Context::Instance()._allocator);

	if (_memory != VK_NULL_HANDLE)
	{
		vkFreeMemory(LogicalDevice::Instance()._device, _memory, Context::Instance()._allocator);
		if (_image != VK_NULL_HANDLE)
			vkDestroyImage(LogicalDevice::Instance()._device, _image, Context::Instance()._allocator);
	}
}

void ImageBuffer::TransitionLayout(const Queue& kQueue, const VkImageLayout kOldLayout, const VkImageLayout kNewLayout) const
{
	ASSERT(kOldLayout != kNewLayout, "kOldLayout is equal to kNewLayout")

	CommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommands(kQueue);

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = kOldLayout;
	barrier.newLayout = kNewLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = _image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (kOldLayout == VK_IMAGE_LAYOUT_UNDEFINED && kNewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (kOldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && kNewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		ASSERT(false, "unsupported layout transition!")
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	CommandBuffer::EndSingleTimeCommands(kQueue, commandBuffer);
}

void ImageBuffer::CopyBuffer(const Queue& kQueue, const Buffer& kBuffer) const
{
	CommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommands(kQueue);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		_size.width,
		_size.height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		kBuffer,
		_image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	CommandBuffer::EndSingleTimeCommands(kQueue, commandBuffer);
}