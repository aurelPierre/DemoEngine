#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "Material.h"

struct Mesh
{
	const std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	Material			_material = VK_NULL_HANDLE;

	VkBuffer			_buffer = VK_NULL_HANDLE;
	VkDeviceMemory		_memory = VK_NULL_HANDLE;
};
