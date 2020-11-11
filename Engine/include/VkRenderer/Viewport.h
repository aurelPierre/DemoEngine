#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "ImageBuffer.h"
#include "CommandBuffer.h"

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

	CommandBuffer			_commandBuffer;

	ImageBuffer				_colorImage;
	ImageBuffer				_depthImage;

	VkFramebuffer			_framebuffer		= VK_NULL_HANDLE;
	VkSampler				_sampler			= VK_NULL_HANDLE;
	VkDescriptorSet			_set				= VK_NULL_HANDLE;

	VkExtent2D				_size;
	VkRenderPass			_renderPass			= VK_NULL_HANDLE;

public:
	Viewport(const VkFormat kFormat, const VkExtent2D kExtent);
	~Viewport();

private:
	void Init(const VkFormat kFormat);
	void Clean();

public:
	void	Resize(const VkFormat kFormat);

	bool	UpdateViewportSize();

	void	StartDraw();
	void	EndDraw();

	void	Render();
};
