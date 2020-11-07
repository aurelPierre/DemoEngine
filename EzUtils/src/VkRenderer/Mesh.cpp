#include "Mesh.h"

#include "LogSystem.h"

#include <unordered_map>

Mesh::Mesh(const Device& kDevice, const std::string kPath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, kPath.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) 
	{
		size_t indexOffset = 0;
		for (size_t i = 0; i < shape.mesh.num_face_vertices.size(); ++i)
		{
			int fv = shape.mesh.num_face_vertices[i];
			if(fv > 3)
				LOG(ez::WARNING, "Mesh " + kPath + "have a face with more than 3 vertices, tangent might be wrong")

			for (size_t j = 0; j < fv; ++j)
			{
				Vertex vertex{};
				tinyobj::index_t idx = shape.mesh.indices[indexOffset + j];

				vertex.pos = {
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2]
				};

				vertex.uv = {
					attrib.texcoords[2 * idx.texcoord_index + 0],
					1.f - attrib.texcoords[2 * idx.texcoord_index + 1]
				};

				vertex.normal = {
					attrib.normals[3 * idx.normal_index + 0],
					attrib.normals[3 * idx.normal_index + 1],
					attrib.normals[3 * idx.normal_index + 2]
				};

				// Shortcuts for vertices
				glm::vec3 v0 = {
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 0].vertex_index + 0],
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 0].vertex_index + 1],
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 0].vertex_index + 2]
				};
				glm::vec3 v1 = {
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 1].vertex_index + 0],
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 1].vertex_index + 1],
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 1].vertex_index + 2]
				};
				glm::vec3 v2 = {
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 2].vertex_index + 0],
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 2].vertex_index + 1],
					attrib.vertices[3 * shape.mesh.indices[indexOffset + 2].vertex_index + 2]
				};

				// Shortcuts for UVs
				glm::vec2 uv0 = {
					attrib.texcoords[2 * shape.mesh.indices[indexOffset + 0].texcoord_index + 0],
					1.f - attrib.texcoords[2 * shape.mesh.indices[indexOffset + 0].texcoord_index + 1]
				};
				glm::vec2 uv1 = {
					attrib.texcoords[2 * shape.mesh.indices[indexOffset + 1].texcoord_index + 0],
					1.f - attrib.texcoords[2 * shape.mesh.indices[indexOffset + 1].texcoord_index + 1]
				};
				glm::vec2 uv2 = {
					attrib.texcoords[2 * shape.mesh.indices[indexOffset + 2].texcoord_index + 0],
					1.f - attrib.texcoords[2 * shape.mesh.indices[indexOffset + 2].texcoord_index + 1]
				};

				// Edges of the triangle : position delta
				glm::vec3 deltaPos1 = v1 - v0;
				glm::vec3 deltaPos2 = v2 - v0;

				// UV delta
				glm::vec2 deltaUV1 = uv1 - uv0;
				glm::vec2 deltaUV2 = uv2 - uv0;

				float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
				//glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x)*r;

				vertex.tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y)*r;
				
				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(_vertices.size());
					_vertices.push_back(vertex);
				}
				else
				{
					_vertices[uniqueVertices[vertex]].tangent += vertex.tangent;
				}

				_indices.push_back(uniqueVertices[vertex]);
			}
			indexOffset += fv;
		}
	}

	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(_indices[0]) * _indices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(LogicalDevice::Instance()._device, &bufferInfo, Context::Instance()._allocator, &_indicesBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(LogicalDevice::Instance()._device, _indicesBuffer, &memRequirements);

		uint32_t type = UINT32_MAX;
		for (uint32_t i = 0; i < kDevice._memoryProperties.memoryTypeCount; i++) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (kDevice._memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				type = i;
			}
		}
		if (type == UINT32_MAX)
			throw;

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = type;

		if (vkAllocateMemory(LogicalDevice::Instance()._device, &allocInfo, Context::Instance()._allocator, &_indicesMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(LogicalDevice::Instance()._device, _indicesBuffer, _indicesMemory, 0);

		void* data;
		vkMapMemory(LogicalDevice::Instance()._device, _indicesMemory, 0,
			sizeof(_indices[0]) * _indices.size(), 0, &data);
		memcpy(data, _indices.data(), (size_t)sizeof(_indices[0]) * _indices.size());
		vkUnmapMemory(LogicalDevice::Instance()._device, _indicesMemory);
	}

	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(_vertices[0]) * _vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(LogicalDevice::Instance()._device, &bufferInfo, Context::Instance()._allocator, &_verticesBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(LogicalDevice::Instance()._device, _verticesBuffer, &memRequirements);

		uint32_t type = UINT32_MAX;
		for (uint32_t i = 0; i < kDevice._memoryProperties.memoryTypeCount; i++) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (kDevice._memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				type = i;
			}
		}
		if (type == UINT32_MAX)
			throw;

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = type;

		if (vkAllocateMemory(LogicalDevice::Instance()._device, &allocInfo, Context::Instance()._allocator, &_verticesMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(LogicalDevice::Instance()._device, _verticesBuffer, _verticesMemory, 0);

		void* data;
		vkMapMemory(LogicalDevice::Instance()._device, _verticesMemory, 0,
			sizeof(_vertices[0]) * _vertices.size(), 0, &data);
		memcpy(data, _vertices.data(), (size_t)sizeof(_vertices[0]) * _vertices.size());
		vkUnmapMemory(LogicalDevice::Instance()._device, _verticesMemory);
	}
}

Mesh::~Mesh()
{
	vkFreeMemory(LogicalDevice::Instance()._device, _indicesMemory, Context::Instance()._allocator);
	vkDestroyBuffer(LogicalDevice::Instance()._device, _indicesBuffer, Context::Instance()._allocator);

	vkFreeMemory(LogicalDevice::Instance()._device, _verticesMemory, Context::Instance()._allocator);
	vkDestroyBuffer(LogicalDevice::Instance()._device, _verticesBuffer, Context::Instance()._allocator);
}

void Mesh::Draw(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		_material->_pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		_material->_pipelineLayout, 0, 1, &_material->_uboSet, 0, nullptr);

	vkCmdBindIndexBuffer(commandBuffer, _indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
	VkDeviceSize offset[]{ 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_verticesBuffer, offset);

	vkCmdDrawIndexed(commandBuffer, _indices.size(), 1, 0, 0, 0);
}
