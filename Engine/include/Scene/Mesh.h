#pragma once

#include <vector>

#include "VkRenderer/Material.h"
#include "VkRenderer/Buffer.h"

class Mesh
{
public:
	std::vector<Vertex>		_vertices;
	std::vector<uint32_t>	_indices;

	Buffer					_indicesBuffer;
	Buffer					_verticesBuffer;

public:
	Mesh(const std::string kPath);
	~Mesh() = default;

public:
	void Draw(const CommandBuffer& commandBuffer) const;

};
