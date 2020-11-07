#pragma once

#include <vulkan/vulkan.h>

#include <vector>

#include "tiny_obj_loader.h"

#include "Device.h"
#include "Material.h"

#include "Buffer.h"

class Mesh
{
public:
	std::vector<Vertex>		_vertices;
	std::vector<uint32_t>	_indices;

	Material*				_material	= nullptr;

	Buffer					_indicesBuffer;
	Buffer					_verticesBuffer;

public:
	Mesh(const Device& kDevice, const std::string kPath);
	~Mesh();

public:
	void Draw(VkCommandBuffer commandBuffer);

};
