#include "Viewport.h"

#include "Core.h"
#include <array>

#include <imgui_impl_vulkan.h>

#include "Mesh.h"


Viewport	CreateViewport(const Context& kContext, const LogicalDevice& kLogicalDevice,
							const Device& kDevice, const VkFormat kFormat, const VkExtent2D kExtent)
{
	Viewport viewport{};

	viewport._size = kExtent;

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = kFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference attachmentRef[2] {};
	attachmentRef[0].attachment = 0;
	attachmentRef[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkFormat depthFormat = findDepthFormat(kDevice._physicalDevice);

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachmentRef[1].attachment = 1;
	attachmentRef[1].layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentRef[0];
	subpass.pDepthStencilAttachment = &attachmentRef[1];

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = attachments.size();
	info.pAttachments = attachments.data();
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = dependencies.size();
	info.pDependencies = dependencies.data();

	VkResult err = vkCreateRenderPass(kLogicalDevice._device, &info, kContext._allocator, &viewport._renderPass);
	check_vk_result(err);

	// Depth attachment
	{
		VkImageCreateInfo image{};
		image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image.imageType = VK_IMAGE_TYPE_2D;
		image.format = depthFormat;
		image.extent.width = viewport._size.width;
		image.extent.height = viewport._size.height;
		image.extent.depth = 1;
		image.mipLevels = 1;
		image.arrayLayers = 1;
		image.samples = VK_SAMPLE_COUNT_1_BIT;
		image.tiling = VK_IMAGE_TILING_OPTIMAL;
		// We will sample directly from the color attachment
		image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

		check_vk_result(vkCreateImage(kLogicalDevice._device, &image, kContext._allocator, &viewport._depthImage));

		VkMemoryAllocateInfo memAlloc{};
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(kLogicalDevice._device, viewport._depthImage, &memReqs);

		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = memReqs.size;

		memAlloc.memoryTypeIndex = findMemoryType(kDevice._memoryProperties, memReqs.memoryTypeBits, 
													VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		check_vk_result(vkAllocateMemory(kLogicalDevice._device, &memAlloc, kContext._allocator, &viewport._depthImageMemory));
		check_vk_result(vkBindImageMemory(kLogicalDevice._device, viewport._depthImage, viewport._depthImageMemory, 0));

		/*** Texture handling ***/
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = depthFormat;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;
		colorAttachmentView.image = viewport._depthImage;

		err = vkCreateImageView(kLogicalDevice._device, &colorAttachmentView, kContext._allocator,
			&viewport._depthImageView);
		check_vk_result(err);
	}

	// Color attachment
	{
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

		check_vk_result(vkCreateImage(kLogicalDevice._device, &image, kContext._allocator, &viewport._colorImage));

		VkMemoryAllocateInfo memAlloc{};
		VkMemoryRequirements memReqs;
		vkGetImageMemoryRequirements(kLogicalDevice._device, viewport._colorImage, &memReqs);

		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = memReqs.size;

		memAlloc.memoryTypeIndex = findMemoryType(kDevice._memoryProperties, memReqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		check_vk_result(vkAllocateMemory(kLogicalDevice._device, &memAlloc, kContext._allocator, &viewport._colorImageMemory));
		check_vk_result(vkBindImageMemory(kLogicalDevice._device, viewport._colorImage, viewport._colorImageMemory, 0));

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
		colorAttachmentView.image = viewport._colorImage;

		err = vkCreateImageView(kLogicalDevice._device, &colorAttachmentView, kContext._allocator,
			&viewport._colorImageView);
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

		VkImageView attachment[2];
		attachment[0] = viewport._colorImageView;
		attachment[1] = viewport._depthImageView;

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = viewport._renderPass;
		info.attachmentCount = 2;
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

	viewport._set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(viewport._sampler, viewport._colorImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	{
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		err = vkCreateFence(kLogicalDevice._device, &info, kContext._allocator, &viewport._fence);
		check_vk_result(err);
	}

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

	VkExtent2D size = { (uint32_t)vMax.x - (uint32_t)vMin.x, (uint32_t)vMax.y - (uint32_t)vMin.y };

	DestroyViewport(kContext, kLogicalDevice, viewport);
	viewport = CreateViewport(kContext, kLogicalDevice, kDevice, kFormat, size);

	ImGui::End();
}

bool	UpdateViewportSize(Viewport& viewport)
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

	return true;
}

void	StartDraw(const LogicalDevice& kLogicalDevice, Viewport& viewport)
{

	VkResult err = vkWaitForFences(kLogicalDevice._device, 1, &viewport._fence, VK_TRUE, UINT64_MAX);
	check_vk_result(err);

	/**********************************************************************************************/

	VkCommandBufferBeginInfo commandBeginInfo = {};
	commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	err = vkBeginCommandBuffer(viewport._commandBuffer, &commandBeginInfo);
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

	VkClearValue clearValues[2];
	clearValues[0].color = { 0.0f, 0.0f, 0.2f, 1.0f };;
	clearValues[1].depthStencil = { 1.0f, 0 };
	
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = viewport._renderPass;
	renderPassBeginInfo.framebuffer = viewport._framebuffer;
	renderPassBeginInfo.renderArea.extent.width = viewport._size.width;
	renderPassBeginInfo.renderArea.extent.height = viewport._size.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(viewport._commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void	EndDraw(const LogicalDevice& kLogicalDevice, Viewport& viewport)
{
	vkCmdEndRenderPass(viewport._commandBuffer);

	VkResult err = vkEndCommandBuffer(viewport._commandBuffer);
	check_vk_result(err);
}

void	Render(const LogicalDevice& kLogicalDevice, Viewport& viewport)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &viewport._commandBuffer;

	submitInfo.signalSemaphoreCount = 0;

	VkResult err = vkResetFences(kLogicalDevice._device, 1, &viewport._fence);
	check_vk_result(err);
	err = vkQueueSubmit(kLogicalDevice._graphicsQueue._queue, 1, &submitInfo, viewport._fence);
	check_vk_result(err);
}

void	DestroyViewport(const Context& kContext, const LogicalDevice& kLogicalDevice, const Viewport& kViewport)
{
	vkDestroyFence(kLogicalDevice._device, kViewport._fence, kContext._allocator);

	vkFreeCommandBuffers(kLogicalDevice._device, kLogicalDevice._graphicsQueue._commandPool, 1, &kViewport._commandBuffer);
	VkResult err = vkFreeDescriptorSets(kLogicalDevice._device, kLogicalDevice._descriptorPool, 1, &kViewport._set);
	check_vk_result(err);

	vkDestroySampler(kLogicalDevice._device, kViewport._sampler, kContext._allocator);
	vkDestroyFramebuffer(kLogicalDevice._device, kViewport._framebuffer, kContext._allocator);

	vkDestroyImageView(kLogicalDevice._device, kViewport._depthImageView, kContext._allocator);
	vkDestroyImage(kLogicalDevice._device, kViewport._depthImage, kContext._allocator);
	vkFreeMemory(kLogicalDevice._device, kViewport._depthImageMemory, kContext._allocator);

	vkDestroyImageView(kLogicalDevice._device, kViewport._colorImageView, kContext._allocator);
	vkDestroyImage(kLogicalDevice._device, kViewport._colorImage, kContext._allocator);
	vkFreeMemory(kLogicalDevice._device, kViewport._colorImageMemory, kContext._allocator);

	vkDestroyRenderPass(kLogicalDevice._device, kViewport._renderPass, kContext._allocator);
}