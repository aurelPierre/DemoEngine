#include "Viewport.h"

#include "Core.h"

Viewport	CreateViewport(const Context& kContext, const LogicalDevice& kLogicalDevice,
							const Device& kDevice, const VkFormat kFormat)
{
	Viewport viewport{};

	VkAttachmentDescription attachment = {};
	attachment.format = kFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference final_attachment = {};
	final_attachment.attachment = 0;
	final_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &final_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	VkResult err = vkCreateRenderPass(kLogicalDevice._device, &info, kContext._allocator, &viewport._renderPass);
	check_vk_result(err);

	{
		// Color attachment
		VkImageCreateInfo image{};
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = VK_FORMAT_R8G8B8A8_SRGB;
		image.extent.width = 512;
		image.extent.height = 512;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// We will sample directly from the color attachment
		image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		check_vk_result(vkCreateImage(kLogicalDevice._device, &image, nullptr, &viewport._image));

		VkMemoryAllocateInfo memAlloc{};
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(kLogicalDevice._device, viewport._image, &memReqs);

		memAlloc.allocationSize = memReqs.size;

		uint32_t type = UINT32_MAX;
		for (uint32_t i = 0; i < kDevice._memoryProperties.memoryTypeCount; i++) {
			if ((memReqs.memoryTypeBits & (1 << i)) && (kDevice._memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				type = i;
			}
		}
		if (type == UINT32_MAX)
			throw;

		memAlloc.memoryTypeIndex = type;

		check_vk_result(vkAllocateMemory(kLogicalDevice._device, &memAlloc, kContext._allocator, &viewport._imageMemory));
		check_vk_result(vkBindImageMemory(kLogicalDevice._device, viewport._image, viewport._imageMemory, 0));

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
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;
		colorAttachmentView.image = viewport._image;

		err = vkCreateImageView(kLogicalDevice._device, &colorAttachmentView, kContext._allocator,
			&viewport._imageView);
		check_vk_result(err);

		VkImageView attachment[1];
		attachment[0] = viewport._imageView;

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = viewport._renderPass;
		info.attachmentCount = 1;
		info.pAttachments = attachment;
		info.width = viewport._size.width;
		info.height = viewport._size.height;
		info.layers = 1;

		{
			err = vkCreateFramebuffer(kLogicalDevice._device, &info, kContext._allocator, &viewport._framebuffer);
			check_vk_result(err);
		}

		{
			VkCommandBufferAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.commandPool = kLogicalDevice._graphicsQueue._commandPool;
			info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount = 1;
			err = vkAllocateCommandBuffers(kLogicalDevice._device, &info, &viewport._commandBuffer);
			check_vk_result(err);
		}
	}

	return viewport;
}

void	DestroyViewport(const Context& kContext, const LogicalDevice& kLogicalDevice, const Viewport& kViewport)
{
	vkDestroySampler(kLogicalDevice._device, kViewport._sampler, kContext._allocator);
	vkDestroyFramebuffer(kLogicalDevice._device, kViewport._framebuffer, kContext._allocator);
	vkDestroyImageView(kLogicalDevice._device, kViewport._imageView, kContext._allocator);
	vkDestroyImage(kLogicalDevice._device, kViewport._image, kContext._allocator);
	vkFreeMemory(kLogicalDevice._device, kViewport._imageMemory, kContext._allocator);
	vkDestroyRenderPass(kLogicalDevice._device, kViewport._renderPass, kContext._allocator);
}