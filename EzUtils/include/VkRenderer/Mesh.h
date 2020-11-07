#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "tiny_obj_loader.h"

#include "Context.h"
#include "Device.h"
#include "Material.h"

class Mesh
{
public:
	std::vector<Vertex>		_vertices;
	std::vector<uint32_t>	_indices;

	Material*				_material	= nullptr;

	VkBuffer				_indicesBuffer		= VK_NULL_HANDLE;
	VkDeviceMemory			_indicesMemory		= VK_NULL_HANDLE;

	VkBuffer				_verticesBuffer		= VK_NULL_HANDLE;
	VkDeviceMemory			_verticesMemory		= VK_NULL_HANDLE;

public:
	Mesh(const Device& kDevice, const std::string kPath);
	~Mesh();

public:
	void Draw(VkCommandBuffer commandBuffer);

};
