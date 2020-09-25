#include "Swapchain.h"

#include "Core.h"
#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"

Swapchain	CreateSwapchain(const Context& kContext, const LogicalDevice& kLogicalDevice,
							const Device& kDevice, const Surface& kSurface,
							const GLFWWindowData* windowData)
{
	Swapchain swapchain{};

	VkSurfaceCapabilitiesKHR surfCaps;
	VkResult err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(kDevice._physicalDevice, kSurface._surface, &surfCaps);
	check_vk_result(err);
	// Get available present modes
	uint32_t presentModeCount;
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(kDevice._physicalDevice, kSurface._surface, &presentModeCount, NULL);
	check_vk_result(err);
	assert(presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(kDevice._physicalDevice, kSurface._surface, &presentModeCount, presentModes.data());
	check_vk_result(err);

	int w, h;
	glfwGetFramebufferSize(windowData->_window, &w, &h);

	// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
	if (surfCaps.currentExtent.width == (uint32_t)-1)
	{
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchain._size.width = w;
		swapchain._size.height = h;
	}
	else
	{
		// TODO check wtf
		// If the surface size is defined, the swap chain size must match
		swapchain._size = surfCaps.currentExtent;
		//*width = surfCaps.currentExtent.width;
		//*height = surfCaps.currentExtent.height;
	}

	// Select a present mode for the swapchain

	// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
	// This mode waits for the vertical blank ("v-sync")
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// Determine the number of images
	uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
	{
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	// Find the transformation of the surface
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfCaps.currentTransform;
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Simply select the first composite alpha format available
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;// kSurface._swapchain;

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.pNext = NULL;
	swapchainCI.surface = kSurface._surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = kSurface._colorFormat;
	swapchainCI.imageColorSpace = kSurface._colorSpace;
	swapchainCI.imageExtent = { swapchain._size.width, swapchain._size.height };
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.queueFamilyIndexCount = 0;
	swapchainCI.pQueueFamilyIndices = NULL;
	swapchainCI.presentMode = swapchainPresentMode;
	//swapchainCI.oldSwapchain = oldSwapchain;
	// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
	swapchainCI.clipped = VK_FALSE;
	swapchainCI.compositeAlpha = compositeAlpha;

	// Enable transfer source on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	err = vkCreateSwapchainKHR(kLogicalDevice._device, &swapchainCI, kContext._allocator, &swapchain._swapchain);
	check_vk_result(err);

	// If an existing swap chain is re-created, destroy the old swap chain
	// This also cleans up all the presentable images
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < swapchain._imageCount; i++)
		{
			vkDestroyImageView(kLogicalDevice._device, swapchain._frames[i]._imageView, nullptr);
		}
		vkDestroySwapchainKHR(kLogicalDevice._device, oldSwapchain, nullptr);
	}

	VkAttachmentDescription attachment = {};
	attachment.format = kSurface._colorFormat;
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

	err = vkCreateRenderPass(kLogicalDevice._device, &info, kContext._allocator, &swapchain._renderPass);
	check_vk_result(err);

	CreateFrames(kContext, kLogicalDevice, kSurface, swapchain);

	return swapchain;
}

void	CreateFrames(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Surface& kSurface, Swapchain& swapchain)
{
	VkResult err = vkGetSwapchainImagesKHR(kLogicalDevice._device, swapchain._swapchain, &swapchain._imageCount, NULL);
	check_vk_result(err);

	// Get the swap chain images
	std::vector<VkImage> images(swapchain._imageCount);
	err = vkGetSwapchainImagesKHR(kLogicalDevice._device, swapchain._swapchain, &swapchain._imageCount, images.data());
	check_vk_result(err);

	swapchain._frames.resize(swapchain._imageCount);
	// Get the swap chain buffers containing the image and imageview
	for (uint32_t i = 0; i < swapchain._imageCount; i++)
	{
		swapchain._frames[i]._image = images[i];

		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = kSurface._colorFormat;
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
		colorAttachmentView.image = swapchain._frames[i]._image;

		err = vkCreateImageView(kLogicalDevice._device, &colorAttachmentView, kContext._allocator,
								&swapchain._frames[i]._imageView);
		check_vk_result(err);

		VkImageView attachment[1];
		attachment[0] = swapchain._frames[i]._imageView;

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = swapchain._renderPass;
		info.attachmentCount = 1;
		info.pAttachments = attachment;
		info.width = swapchain._size.width;
		info.height = swapchain._size.height;
		info.layers = 1;

		{
			err = vkCreateFramebuffer(kLogicalDevice._device, &info, kContext._allocator, &swapchain._frames[i]._framebuffer);
			check_vk_result(err);
		}

		{
			VkCommandBufferAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.commandPool = kLogicalDevice._graphicsQueue._commandPool;
			info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount = 1;
			err = vkAllocateCommandBuffers(kLogicalDevice._device, &info, &swapchain._frames[i]._commandBuffer);
			check_vk_result(err);
		}
		{
			VkFenceCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			err = vkCreateFence(kLogicalDevice._device, &info, kContext._allocator, &swapchain._frames[i]._fence);
			check_vk_result(err);
		}
		{
			VkSemaphoreCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			err = vkCreateSemaphore(kLogicalDevice._device, &info, kContext._allocator, &swapchain._frames[i]._presentComplete);
			check_vk_result(err);
			err = vkCreateSemaphore(kLogicalDevice._device, &info, kContext._allocator, &swapchain._frames[i]._renderComplete);
			check_vk_result(err);
		}

	}
}

void	AddViewport(const Viewport& viewport, Swapchain& swapchain)
{
	swapchain._viewports.emplace_back(viewport);
}

void	ResizeSwapchain(const Context& kContext, const LogicalDevice& kLogicalDevice,
	const Device& kDevice, const Surface& kSurface,
	const GLFWWindowData* windowData, Swapchain& swapchain)
{
	vkDeviceWaitIdle(kLogicalDevice._device);
	DestroySwapchain(kContext, kLogicalDevice, swapchain);
	swapchain = CreateSwapchain(kContext, kLogicalDevice, kDevice, kSurface, windowData);
}

bool	Draw(const LogicalDevice& kLogicalDevice, Swapchain& swapchain)
{
	vkWaitForFences(kLogicalDevice._device, 1, &swapchain._frames[swapchain._currentFrame]._fence, VK_TRUE, UINT64_MAX);

	VkResult err = vkAcquireNextImageKHR(kLogicalDevice._device, swapchain._swapchain, UINT64_MAX,
											swapchain._frames[swapchain._currentFrame]._presentComplete,
											VK_NULL_HANDLE, &swapchain._currentFrame);
	if (err == VK_ERROR_OUT_OF_DATE_KHR)
		return false;
	check_vk_result(err);

	for (int i = 0; i < swapchain._viewports.size(); ++i)
		Draw(kLogicalDevice, swapchain._viewports[i]);

	VkCommandBufferBeginInfo commandBeginInfo = {};
	commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	err = vkBeginCommandBuffer(swapchain._frames[swapchain._currentFrame]._commandBuffer, &commandBeginInfo);
	check_vk_result(err);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = swapchain._size.width;
	viewport.height = swapchain._size.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { swapchain._size.width, swapchain._size.height };

	vkCmdSetViewport(swapchain._frames[swapchain._currentFrame]._commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(swapchain._frames[swapchain._currentFrame]._commandBuffer, 0, 1, &scissor);

	VkClearValue clearColor{};
	clearColor.color = { 0.0f, 0.0f, 0.0f, 0.0f };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = swapchain._renderPass;
	renderPassBeginInfo.framebuffer = swapchain._frames[swapchain._currentFrame]._framebuffer;
	renderPassBeginInfo.renderArea.extent.width = swapchain._size.width;
	renderPassBeginInfo.renderArea.extent.height = swapchain._size.height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(swapchain._frames[swapchain._currentFrame]._commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	/**********************************************/
	/* Foreach objects in this renderpass to draw */
	/**********************************************/


	// EDITOR
	{
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, swapchain._frames[swapchain._currentFrame]._commandBuffer);
	}

	vkCmdEndRenderPass(swapchain._frames[swapchain._currentFrame]._commandBuffer);

	err = vkEndCommandBuffer(swapchain._frames[swapchain._currentFrame]._commandBuffer);
	check_vk_result(err);

	return true;
}

void	Render(const LogicalDevice& kLogicalDevice, Swapchain& swapchain)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { swapchain._frames[swapchain._currentFrame]._presentComplete };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	std::vector<VkCommandBuffer> commands(swapchain._viewports.size() + 1);
	commands[0] = swapchain._frames[swapchain._currentFrame]._commandBuffer;
	for(int i = 0; i < swapchain._viewports.size(); ++i)
		commands[i + 1] = swapchain._viewports[i]._commandBuffer;

	submitInfo.commandBufferCount = commands.size();
	submitInfo.pCommandBuffers = commands.data();

	VkSemaphore signalSemaphores[] = { swapchain._frames[swapchain._currentFrame]._renderComplete };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(kLogicalDevice._device, 1, &swapchain._frames[swapchain._currentFrame]._fence);
	VkResult err = vkQueueSubmit(kLogicalDevice._graphicsQueue._queue, 1, &submitInfo,
									swapchain._frames[swapchain._currentFrame]._fence);
	check_vk_result(err);
}

bool	Present(const LogicalDevice& kLogicalDevice, Swapchain& swapchain)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &swapchain._frames[swapchain._currentFrame]._renderComplete;

	VkSwapchainKHR swapChains[] = { swapchain._swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &swapchain._currentFrame;

	presentInfo.pResults = nullptr; // Optional

	VkResult err = vkQueuePresentKHR(kLogicalDevice._graphicsQueue._queue, &presentInfo);

	swapchain._currentFrame = (swapchain._currentFrame + 1) % swapchain._frames.size();
	
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		return false;
	else if (err != VK_SUCCESS)
		check_vk_result(err);

	return true;
}

void	DestroyFrame(const Context& kContext, const LogicalDevice& kLogicalDevice, const Frame& kFrame)
{
	vkDestroySemaphore(kLogicalDevice._device, kFrame._presentComplete, kContext._allocator);
	vkDestroySemaphore(kLogicalDevice._device, kFrame._renderComplete, kContext._allocator);

	vkDestroyFence(kLogicalDevice._device, kFrame._fence, kContext._allocator);
	vkDestroyFramebuffer(kLogicalDevice._device, kFrame._framebuffer, kContext._allocator);
	vkDestroyImageView(kLogicalDevice._device, kFrame._imageView, kContext._allocator);
}

void	DestroySwapchain(const Context& kContext, const LogicalDevice& kLogicalDevice, const Swapchain& kSwapchain)
{
	for (uint32_t i = 0; i < kSwapchain._frames.size(); ++i)
		DestroyFrame(kContext, kLogicalDevice, kSwapchain._frames[i]);

	vkDestroyRenderPass(kLogicalDevice._device, kSwapchain._renderPass, kContext._allocator);
	vkDestroySwapchainKHR(kLogicalDevice._device, kSwapchain._swapchain, kContext._allocator);
}