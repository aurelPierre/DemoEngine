#include "Swapchain.h"

#include "Context.h"
#include "Core.h"
#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"

Frame::Frame(const VkImage kImage, const VkFormat kFormat, const VkExtent2D& kExtent, const VkRenderPass kRenderPass)
	: _commandBuffer{ LogicalDevice::Instance()._graphicsQueue }, _imageBuffer { kImage, kFormat, kExtent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }
{
	ASSERT(kImage != nullptr, "kImage is nullptr")
	ASSERT(kExtent.width != 0u || kExtent.height != 0, "kExtent.width is 0 or kExtent.height is 0")
	ASSERT(kRenderPass != nullptr, "kRenderPass is nullptr")

	VkImageView attachment[1];
	attachment[0] = _imageBuffer._view;

	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = kRenderPass;
	info.attachmentCount = 1;
	info.pAttachments = attachment;
	info.width = kExtent.width;
	info.height = kExtent.height;
	info.layers = 1;

	VkResult err = vkCreateFramebuffer(LogicalDevice::Instance()._device, &info, Context::Instance()._allocator, &_framebuffer);
	check_vk_result(err);

	{
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		err = vkCreateFence(LogicalDevice::Instance()._device, &info, Context::Instance()._allocator, &_fence);
		check_vk_result(err);
	}
	{
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		err = vkCreateSemaphore(LogicalDevice::Instance()._device, &info, Context::Instance()._allocator, &_presentComplete);
		check_vk_result(err);
		err = vkCreateSemaphore(LogicalDevice::Instance()._device, &info, Context::Instance()._allocator, &_renderComplete);
		check_vk_result(err);
	}
}

Frame::~Frame()
{
	Clean();
}

Frame::Frame(Frame&& frame)
	: _commandBuffer{ std::move(frame._commandBuffer) }, _imageBuffer{ std::move(frame._imageBuffer) }, _framebuffer { frame._framebuffer },
	_fence{ frame._fence }, _presentComplete { frame._presentComplete }, _renderComplete{ frame._renderComplete }
{
	frame._framebuffer		= VK_NULL_HANDLE;

	frame._fence			= VK_NULL_HANDLE;
	frame._presentComplete	= VK_NULL_HANDLE;
	frame._renderComplete	= VK_NULL_HANDLE;
}

Frame& Frame::operator=(Frame&& frame)
{
	Clean();

	_commandBuffer		= std::move(frame._commandBuffer);
	_imageBuffer		= std::move(frame._imageBuffer);
	_framebuffer		= frame._framebuffer;

	_fence				= frame._fence;
	_presentComplete	= frame._presentComplete;
	_renderComplete		= frame._renderComplete;

	frame._framebuffer		= VK_NULL_HANDLE;

	frame._fence			= VK_NULL_HANDLE;
	frame._presentComplete	= VK_NULL_HANDLE;
	frame._renderComplete	= VK_NULL_HANDLE;

	return *this;
}

void Frame::Clean()
{
	if (_presentComplete != VK_NULL_HANDLE)
		vkDestroySemaphore(LogicalDevice::Instance()._device, _presentComplete, Context::Instance()._allocator);
	if (_renderComplete != VK_NULL_HANDLE)
		vkDestroySemaphore(LogicalDevice::Instance()._device, _renderComplete, Context::Instance()._allocator);
	if (_fence != VK_NULL_HANDLE)
		vkDestroyFence(LogicalDevice::Instance()._device, _fence, Context::Instance()._allocator);

	if (_framebuffer != VK_NULL_HANDLE)
		vkDestroyFramebuffer(LogicalDevice::Instance()._device, _framebuffer, Context::Instance()._allocator);
}

Swapchain::Swapchain(const Device& kDevice, const Surface& kSurface, const GLFWWindowData* windowData)
{
	ASSERT(windowData != nullptr, "windowData is nullptr")

	Init(kDevice, kSurface, windowData);
}

Swapchain::~Swapchain()
{
	Clean();
}

void Swapchain::Init(const Device& kDevice, const Surface& kSurface, const GLFWWindowData* windowData)
{
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
		_size.width = w;
		_size.height = h;
	}
	else
	{
		// TODO check wtf
		// If the surface size is defined, the swap chain size must match
		_size = surfCaps.currentExtent;
		//*width = surfCaps.currentExtent.width;
		//*height = surfCaps.currentExtent.height;
	}

	// Select a present mode for the swapchain

	// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
	// This mode waits for the vertical blank ("v-sync")
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

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

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.pNext = NULL;
	swapchainCI.surface = kSurface._surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = kSurface._colorFormat;
	swapchainCI.imageColorSpace = kSurface._colorSpace;
	swapchainCI.imageExtent = { _size.width, _size.height };
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

	err = vkCreateSwapchainKHR(LogicalDevice::Instance()._device, &swapchainCI, Context::Instance()._allocator, &_swapchain);
	check_vk_result(err);

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
	dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	err = vkCreateRenderPass(LogicalDevice::Instance()._device, &info, Context::Instance()._allocator, &_renderPass);
	check_vk_result(err);

	err = vkGetSwapchainImagesKHR(LogicalDevice::Instance()._device, _swapchain, &_imageCount, NULL);
	check_vk_result(err);

	// Get the swap chain images
	std::vector<VkImage> images(_imageCount);
	err = vkGetSwapchainImagesKHR(LogicalDevice::Instance()._device, _swapchain, &_imageCount, images.data());
	check_vk_result(err);

	_frames.reserve(_imageCount);
	// Get the swap chain buffers containing the image and imageview
	for (uint32_t i = 0; i < _imageCount; i++)
		_frames.emplace_back(images[i], kSurface._colorFormat, _size, _renderPass);
}

void Swapchain::Clean()
{
	vkDeviceWaitIdle(LogicalDevice::Instance()._device);

	_frames.clear();

	vkDestroyRenderPass(LogicalDevice::Instance()._device, _renderPass, Context::Instance()._allocator);
	vkDestroySwapchainKHR(LogicalDevice::Instance()._device, _swapchain, Context::Instance()._allocator);
}

void Swapchain::Resize(const Device& kDevice, const Surface& kSurface, const GLFWWindowData* windowData)
{
	ASSERT(windowData != nullptr, "windowData is nullptr")

	Clean();
	Init(kDevice, kSurface, windowData);
}

bool Swapchain::AcquireNextImage()
{
	vkWaitForFences(LogicalDevice::Instance()._device, 1, &_frames[_currentFrame]._fence, VK_TRUE, UINT64_MAX);

	VkResult err = vkAcquireNextImageKHR(LogicalDevice::Instance()._device, _swapchain, UINT64_MAX,
										_frames[_currentFrame]._presentComplete, VK_NULL_HANDLE, &_currentFrame);

	if (err == VK_ERROR_OUT_OF_DATE_KHR)
		return false;

	check_vk_result(err);
	return true;
}

void Swapchain::Draw()
{
	_frames[_currentFrame]._commandBuffer.Begin();

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = _size.width;
	viewport.height = _size.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { _size.width, _size.height };

	vkCmdSetViewport(_frames[_currentFrame]._commandBuffer._commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(_frames[_currentFrame]._commandBuffer._commandBuffer, 0, 1, &scissor);

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = _renderPass;
	renderPassBeginInfo.framebuffer = _frames[_currentFrame]._framebuffer;
	renderPassBeginInfo.renderArea.extent.width = _size.width;
	renderPassBeginInfo.renderArea.extent.height = _size.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(_frames[_currentFrame]._commandBuffer._commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	/**********************************************/
	/* Foreach objects in this renderpass to draw */
	/**********************************************/


	// EDITOR
	{
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _frames[_currentFrame]._commandBuffer._commandBuffer);
	}

	vkCmdEndRenderPass(_frames[_currentFrame]._commandBuffer._commandBuffer);

	_frames[_currentFrame]._commandBuffer.End();
}

void Swapchain::Render()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _frames[_currentFrame]._presentComplete };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	std::vector<VkCommandBuffer> commands(1);
	commands[0] = _frames[_currentFrame]._commandBuffer._commandBuffer;

	submitInfo.commandBufferCount = commands.size();
	submitInfo.pCommandBuffers = commands.data();

	VkSemaphore signalSemaphores[] = { _frames[_currentFrame]._renderComplete };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(LogicalDevice::Instance()._device, 1, &_frames[_currentFrame]._fence);
	VkResult err = vkQueueSubmit(LogicalDevice::Instance()._graphicsQueue._queue, 1, &submitInfo,
									_frames[_currentFrame]._fence);
	check_vk_result(err);
}

bool Swapchain::Present()
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_frames[_currentFrame]._renderComplete;

	VkSwapchainKHR swapChains[] = { _swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &_currentFrame;

	presentInfo.pResults = nullptr; // Optional

	VkResult err = vkQueuePresentKHR(LogicalDevice::Instance()._graphicsQueue._queue, &presentInfo);

	_currentFrame = (_currentFrame + 1) % _frames.size();
	
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		return false;
	else if (err != VK_SUCCESS)
		check_vk_result(err);

	return true;
}
