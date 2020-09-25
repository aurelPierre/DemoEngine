#pragma once

#include <vulkan/vulkan.h>

#include "Context.h"
#include "Device.h"

struct Viewport
{
	VkCommandBuffer			_commandBuffer	= VK_NULL_HANDLE;
	VkDeviceMemory			_imageMemory	= VK_NULL_HANDLE;
	VkImage					_image			= VK_NULL_HANDLE;
	VkImageView				_imageView		= VK_NULL_HANDLE;
	VkFramebuffer			_framebuffer	= VK_NULL_HANDLE;
	VkSampler				_sampler		= VK_NULL_HANDLE;
	VkDescriptorSet			_set			= VK_NULL_HANDLE;

	VkExtent2D				_size;
	VkRenderPass			_renderPass		= VK_NULL_HANDLE;
};

Viewport	CreateViewport(const Context& kContext, const LogicalDevice& kLogicalDevice,
							const Device& kDevice, const VkFormat kFormat, const VkExtent2D kExtent);

void	ResizeViewport(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Device& kDevice, const VkFormat kFormat, Viewport& viewport);

bool	Draw(const LogicalDevice& kLogicalDevice, Viewport& viewport);

void	DestroyViewport(const Context& kContext, const LogicalDevice& kLogicalDevice, const Viewport& kViewport);