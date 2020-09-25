#include "Viewport.h"

#include "Core.h"

#include <imgui_impl_vulkan.h>

Viewport	CreateViewport(const Context& kContext, const LogicalDevice& kLogicalDevice,
							const Device& kDevice, const VkFormat kFormat, const VkExtent2D kExtent)
{
	Viewport viewport{};

	viewport._size = kExtent;

	VkAttachmentDescription attachment = {};
	attachment.format = kFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

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
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = kFormat;
		image.extent.width = viewport._size.width;
		image.extent.height = viewport._size.height;
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

		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
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

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerInfo.addressModeV = samplerInfo.addressModeU;
		samplerInfo.addressModeW = samplerInfo.addressModeU;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.maxAnisotropy = 1.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		check_vk_result(vkCreateSampler(kLogicalDevice._device, &samplerInfo, kContext._allocator, &viewport._sampler));

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

	viewport._set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(viewport._sampler, viewport._imageView,					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return viewport;
}

void	ResizeViewport(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Device& kDevice, const VkFormat kFormat, Viewport& viewport)
{
	vkDeviceWaitIdle(kLogicalDevice._device);

	ImVec2 vMin = ImGui::GetWindowContentRegionMin();
	ImVec2 vMax = ImGui::GetWindowContentRegionMax();

	vMin.x += ImGui::GetWindowPos().x;
	vMin.y += ImGui::GetWindowPos().y;
	vMax.x += ImGui::GetWindowPos().x;
	vMax.y += ImGui::GetWindowPos().y;

	ImVec2 size = { vMax.x - vMin.x, vMax.y - vMin.y };
	DestroyViewport(kContext, kLogicalDevice, viewport);
	viewport = CreateViewport(kContext, kLogicalDevice, kDevice, kFormat, { (uint32_t)size.x, (uint32_t)size.y });
	ImGui::End();
}

bool	Draw(const LogicalDevice& kLogicalDevice, Viewport& viewport)
{
	ImGui::Begin("Viewport");

	ImVec2 vMin = ImGui::GetWindowContentRegionMin();
	ImVec2 vMax = ImGui::GetWindowContentRegionMax();

	vMin.x += ImGui::GetWindowPos().x;
	vMin.y += ImGui::GetWindowPos().y;
	vMax.x += ImGui::GetWindowPos().x;
	vMax.y += ImGui::GetWindowPos().y;

	ImVec2 size = { vMax.x - vMin.x, vMax.y - vMin.y };
	if (size.x != viewport._size.width || size.y != viewport._size.height)
		return false;

	ImGui::Image(viewport._set, size);
	ImGui::End();

	VkCommandBufferBeginInfo commandBeginInfo = {};
	commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult err = vkBeginCommandBuffer(viewport._commandBuffer, &commandBeginInfo);
	check_vk_result(err);

	VkViewport vkViewport = {};
	vkViewport.x = 0.0f;
	vkViewport.y = 0.0f;
	vkViewport.width = viewport._size.width;
	vkViewport.height = viewport._size.height;
	vkViewport.minDepth = 0.0f;
	vkViewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { viewport._size.width, viewport._size.height };

	vkCmdSetViewport(viewport._commandBuffer, 0, 1, &vkViewport);
	vkCmdSetScissor(viewport._commandBuffer, 0, 1, &scissor);

	VkClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 1.f, 1.0f };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = viewport._renderPass;
	renderPassBeginInfo.framebuffer = viewport._framebuffer;
	renderPassBeginInfo.renderArea.extent.width = viewport._size.width;
	renderPassBeginInfo.renderArea.extent.height = viewport._size.height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(viewport._commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	/**********************************************/
	/* Foreach objects in this renderpass to draw */
	/**********************************************/

	vkCmdEndRenderPass(viewport._commandBuffer);

	err = vkEndCommandBuffer(viewport._commandBuffer);
	check_vk_result(err);

	return true;
}

void	DestroyViewport(const Context& kContext, const LogicalDevice& kLogicalDevice, const Viewport& kViewport)
{
	vkFreeCommandBuffers(kLogicalDevice._device, kLogicalDevice._graphicsQueue._commandPool, 1, &kViewport._commandBuffer);
	vkFreeDescriptorSets(kLogicalDevice._device, kLogicalDevice._descriptorPool, 1, &kViewport._set);

	vkDestroySampler(kLogicalDevice._device, kViewport._sampler, kContext._allocator);
	vkDestroyFramebuffer(kLogicalDevice._device, kViewport._framebuffer, kContext._allocator);
	vkDestroyImageView(kLogicalDevice._device, kViewport._imageView, kContext._allocator);
	vkDestroyImage(kLogicalDevice._device, kViewport._image, kContext._allocator);
	vkFreeMemory(kLogicalDevice._device, kViewport._imageMemory, kContext._allocator);
	vkDestroyRenderPass(kLogicalDevice._device, kViewport._renderPass, kContext._allocator);
}