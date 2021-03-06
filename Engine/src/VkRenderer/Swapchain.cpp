#include "VkRenderer/Swapchain.h"

#include "Core.h"
#include "VkRenderer/Context.h"

#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"

FrameData::FrameData()
	: _commandBuffer{ LogicalDevice::Instance()._graphicsQueue }
{
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkResult err = vkCreateFence(LogicalDevice::Instance()._device, &fenceInfo, Context::Instance()._allocator, &_fence);
	VK_ASSERT(err, "error when creating fence");

	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	err = vkCreateSemaphore(LogicalDevice::Instance()._device, &info, Context::Instance()._allocator, &_presentComplete);
	VK_ASSERT(err, "error when creating semaphore");

	err = vkCreateSemaphore(LogicalDevice::Instance()._device, &info, Context::Instance()._allocator, &_renderComplete);
	VK_ASSERT(err, "error when creating semaphore");
}

FrameData::~FrameData()
{
	Clean();
}

FrameData::FrameData(FrameData&& frameImage)
	: _commandBuffer{ std::move(frameImage._commandBuffer) }, _fence{ frameImage._fence },
	_presentComplete{ frameImage._presentComplete }, _renderComplete{ frameImage._renderComplete }
{
	frameImage._fence = VK_NULL_HANDLE;
	frameImage._presentComplete = VK_NULL_HANDLE;
	frameImage._renderComplete = VK_NULL_HANDLE;
}

FrameData& FrameData::operator=(FrameData&& frameImage)
{
	Clean();

	_commandBuffer = std::move(frameImage._commandBuffer);

	_fence = frameImage._fence;
	_presentComplete = frameImage._presentComplete;
	_renderComplete = frameImage._renderComplete;

	frameImage._fence = VK_NULL_HANDLE;
	frameImage._presentComplete = VK_NULL_HANDLE;
	frameImage._renderComplete = VK_NULL_HANDLE;

	return *this;
}

void FrameData::Clean()
{
	if (_presentComplete != VK_NULL_HANDLE)
		vkDestroySemaphore(LogicalDevice::Instance()._device, _presentComplete, Context::Instance()._allocator);
	if (_renderComplete != VK_NULL_HANDLE)
		vkDestroySemaphore(LogicalDevice::Instance()._device, _renderComplete, Context::Instance()._allocator);
	if (_fence != VK_NULL_HANDLE)
		vkDestroyFence(LogicalDevice::Instance()._device, _fence, Context::Instance()._allocator);
}

FrameImage::FrameImage(const VkImage kImage, const VkFormat kFormat, const VkExtent2D& kExtent, const VkRenderPass kRenderPass)
	: _imageBuffer { kImage, kFormat, kExtent, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT }
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
	VK_ASSERT(err, "error when creating framebuffer");
}

FrameImage::~FrameImage()
{
	Clean();
}

FrameImage::FrameImage(FrameImage&& frameImage)
	: _imageBuffer{ std::move(frameImage._imageBuffer) }, _framebuffer { frameImage._framebuffer }
{
	frameImage._framebuffer		= VK_NULL_HANDLE;
}

FrameImage& FrameImage::operator=(FrameImage&& frameImage)
{
	Clean();

	_imageBuffer		= std::move(frameImage._imageBuffer);
	_framebuffer		= frameImage._framebuffer;

	frameImage._framebuffer		= VK_NULL_HANDLE;

	return *this;
}

void FrameImage::Clean()
{
	if (_framebuffer != VK_NULL_HANDLE)
		vkDestroyFramebuffer(LogicalDevice::Instance()._device, _framebuffer, Context::Instance()._allocator);
}

Swapchain::Swapchain(const Surface& kSurface, const GLFWWindowData* windowData)
{
	ASSERT(windowData != nullptr, "windowData is nullptr")

	Init(kSurface, windowData);
}

Swapchain::~Swapchain()
{
	Clean();
}

void Swapchain::Init(const Surface& kSurface, const GLFWWindowData* windowData)
{
	VkSurfaceCapabilitiesKHR surfCaps;
	VkResult err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(LogicalDevice::Instance()._physicalDevice->_physicalDevice, kSurface._surface, &surfCaps);
	VK_ASSERT(err, "error when getting physical device surface capabilities KHR");
	// Get available present modes
	uint32_t presentModeCount;
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(LogicalDevice::Instance()._physicalDevice->_physicalDevice, kSurface._surface, &presentModeCount, NULL);
	VK_ASSERT(err, "error when getting physical device surface present modes KHR");
	assert(presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(LogicalDevice::Instance()._physicalDevice->_physicalDevice, kSurface._surface, &presentModeCount, presentModes.data());
	VK_ASSERT(err, "error when getting physical device surface present modes KHR");

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
	VK_ASSERT(err, "error when creating swapchain KHR");

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
	VK_ASSERT(err, "error when creating render pass");

	err = vkGetSwapchainImagesKHR(LogicalDevice::Instance()._device, _swapchain, &_imageCount, NULL);
	VK_ASSERT(err, "error when getting swapchain images KHR");

	// Get the swap chain images
	std::vector<VkImage> images(_imageCount);
	err = vkGetSwapchainImagesKHR(LogicalDevice::Instance()._device, _swapchain, &_imageCount, images.data());
	VK_ASSERT(err, "error when gettings swapchain images KHR");

	_framesData.reserve(_imageCount);
	_framesImage.reserve(_imageCount);
	// Get the swap chain buffers containing the image and imageview
	for (uint32_t i = 0; i < _imageCount; i++)
	{
		_framesData.emplace_back();
		_framesImage.emplace_back(images[i], kSurface._colorFormat, _size, _renderPass);
	}
}

void Swapchain::Clean()
{
	vkDeviceWaitIdle(LogicalDevice::Instance()._device);

	_framesImage.clear();

	vkDestroyRenderPass(LogicalDevice::Instance()._device, _renderPass, Context::Instance()._allocator);
	vkDestroySwapchainKHR(LogicalDevice::Instance()._device, _swapchain, Context::Instance()._allocator);
}

void Swapchain::Resize(const Surface& kSurface, const GLFWWindowData* windowData)
{
	ASSERT(windowData != nullptr, "windowData is nullptr")

	Clean();
	Init(kSurface, windowData);
}

bool Swapchain::AcquireNextImage()
{
	vkWaitForFences(LogicalDevice::Instance()._device, 1, &_framesData[_currentFrame]._fence, VK_TRUE, UINT64_MAX);

	VkResult err = vkAcquireNextImageKHR(LogicalDevice::Instance()._device, _swapchain, UINT64_MAX,
											_framesData[_currentFrame]._presentComplete, VK_NULL_HANDLE, &_currentImage);

	if (err == VK_ERROR_OUT_OF_DATE_KHR)
		return false;

	VK_ASSERT(err, "error when acquiring next image KHR");
	return true;
}

void Swapchain::Draw()
{
	_framesData[_currentFrame]._commandBuffer.Begin();

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

	vkCmdSetViewport(_framesData[_currentFrame]._commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(_framesData[_currentFrame]._commandBuffer, 0, 1, &scissor);

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.0f, 0.0f, 0.f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = _renderPass;
	renderPassBeginInfo.framebuffer = _framesImage[_currentImage]._framebuffer;
	renderPassBeginInfo.renderArea.extent.width = _size.width;
	renderPassBeginInfo.renderArea.extent.height = _size.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	vkCmdBeginRenderPass(_framesData[_currentFrame]._commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	/**********************************************/
	/* Foreach objects in this renderpass to draw */
	/**********************************************/


	// EDITOR
	{
		ImGui::Render();
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, _framesData[_currentFrame]._commandBuffer);
	}

	vkCmdEndRenderPass(_framesData[_currentFrame]._commandBuffer);

	_framesData[_currentFrame]._commandBuffer.End();
}

void Swapchain::Render()
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _framesData[_currentFrame]._presentComplete };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	std::vector<VkCommandBuffer> commands(1);
	commands[0] = _framesData[_currentFrame]._commandBuffer;

	submitInfo.commandBufferCount = commands.size();
	submitInfo.pCommandBuffers = commands.data();

	VkSemaphore signalSemaphores[] = { _framesData[_currentFrame]._renderComplete };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(LogicalDevice::Instance()._device, 1, &_framesData[_currentFrame]._fence);
	VkResult err = vkQueueSubmit(LogicalDevice::Instance()._graphicsQueue._queue, 1, &submitInfo,
		_framesData[_currentFrame]._fence);
	VK_ASSERT(err, "error when submitting queue");
}

bool Swapchain::Present()
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_framesData[_currentFrame]._renderComplete;

	VkSwapchainKHR swapChains[] = { _swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &_currentFrame;

	presentInfo.pResults = nullptr; // Optional

	VkResult err = vkQueuePresentKHR(LogicalDevice::Instance()._graphicsQueue._queue, &presentInfo);

	_currentFrame = (_currentFrame + 1) % _framesData.size();
	
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		return false;
	else if (err != VK_SUCCESS)
		VK_ASSERT(err, "error when presenting queue KHR");

	return true;
}
