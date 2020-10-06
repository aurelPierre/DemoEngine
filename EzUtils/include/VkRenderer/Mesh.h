#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "tiny_obj_loader.h"

#include "Context.h"
#include "Device.h"
#include "Material.h"

struct Mesh
{
	std::vector<Vertex>		_vertices;
	std::vector<uint32_t>	_indices;

	Material*				_material	= nullptr;

	VkBuffer				_indicesBuffer		= VK_NULL_HANDLE;
	VkDeviceMemory			_indicesMemory		= VK_NULL_HANDLE;

	VkBuffer				_verticesBuffer		= VK_NULL_HANDLE;
	VkDeviceMemory			_verticesMemory		= VK_NULL_HANDLE;
};

Mesh	CreateMesh(const Context& kContext, const LogicalDevice& kLogicalDevice, const Device& kDevice,
					const std::string kPath);

void	Draw(const LogicalDevice& kLogicalDevice, const Mesh& kMesh, VkCommandBuffer commandBuffer);

void	DestroyMesh(const Context& kContext, const LogicalDevice& kLogicalDevice, const Mesh& kMesh);