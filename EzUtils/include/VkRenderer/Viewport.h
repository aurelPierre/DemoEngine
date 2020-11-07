#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "Context.h"
#include "Device.h"

class Mesh;

struct UniformBufferObject {
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

class Viewport
{
public:
	VkFence					_fence				= VK_NULL_HANDLE;

	VkCommandBuffer			_commandBuffer		= VK_NULL_HANDLE;

	VkDeviceMemory			_colorImageMemory	= VK_NULL_HANDLE;
	VkImage					_colorImage			= VK_NULL_HANDLE;
	VkImageView				_colorImageView		= VK_NULL_HANDLE;

	VkDeviceMemory			_depthImageMemory	= VK_NULL_HANDLE;
	VkImage					_depthImage			= VK_NULL_HANDLE;
	VkImageView				_depthImageView		= VK_NULL_HANDLE;

	VkFramebuffer			_framebuffer		= VK_NULL_HANDLE;
	VkSampler				_sampler			= VK_NULL_HANDLE;
	VkDescriptorSet			_set				= VK_NULL_HANDLE;

	VkExtent2D				_size;
	VkRenderPass			_renderPass			= VK_NULL_HANDLE;

public:
	Viewport(const Device& kDevice, const VkFormat kFormat, const VkExtent2D kExtent);
	~Viewport();

public:
	bool	UpdateViewportSize();

	void	StartDraw();
	void	EndDraw();

	void	Render();
};

void	ResizeViewport(const Context& kContext, const LogicalDevice& kLogicalDevice,
						const Device& kDevice, const VkFormat kFormat, Viewport& viewport);
