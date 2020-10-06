#include "Mesh.h"

#include <unordered_map>

Mesh	CreateMesh(const Context& kContext, const LogicalDevice& kLogicalDevice, const Device& kDevice,
					const std::string kPath)
{
	Mesh mesh{};

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, kPath.c_str())) {
		throw std::runtime_error(warn + err);
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(mesh._vertices.size());
				mesh._vertices.push_back(vertex);
			}

			mesh._indices.push_back(uniqueVertices[vertex]);
		}
	}

	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(mesh._indices[0]) * mesh._indices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(kLogicalDevice._device, &bufferInfo, kContext._allocator, &mesh._indicesBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(kLogicalDevice._device, mesh._indicesBuffer, &memRequirements);

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

		if (vkAllocateMemory(kLogicalDevice._device, &allocInfo, kContext._allocator, &mesh._indicesMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(kLogicalDevice._device, mesh._indicesBuffer, mesh._indicesMemory, 0);

		void* data;
		vkMapMemory(kLogicalDevice._device, mesh._indicesMemory, 0,
			sizeof(mesh._indices[0]) * mesh._indices.size(), 0, &data);
		memcpy(data, mesh._indices.data(), (size_t)sizeof(mesh._indices[0]) * mesh._indices.size());
		vkUnmapMemory(kLogicalDevice._device, mesh._indicesMemory);
	}

	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(mesh._vertices[0]) * mesh._vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(kLogicalDevice._device, &bufferInfo, kContext._allocator, &mesh._verticesBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(kLogicalDevice._device, mesh._verticesBuffer, &memRequirements);

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

		if (vkAllocateMemory(kLogicalDevice._device, &allocInfo, kContext._allocator, &mesh._verticesMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(kLogicalDevice._device, mesh._verticesBuffer, mesh._verticesMemory, 0);

		void* data;
		vkMapMemory(kLogicalDevice._device, mesh._verticesMemory, 0,
			sizeof(mesh._vertices[0]) * mesh._vertices.size(), 0, &data);
		memcpy(data, mesh._vertices.data(), (size_t)sizeof(mesh._vertices[0]) * mesh._vertices.size());
		vkUnmapMemory(kLogicalDevice._device, mesh._verticesMemory);
	}

	return mesh;
}

void	Draw(const LogicalDevice& kLogicalDevice, const Mesh& kMesh, VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		kMesh._material->_pipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		kMesh._material->_pipelineLayout, 0, 1, &kMesh._material->_uboSet, 0, nullptr);

	vkCmdBindIndexBuffer(commandBuffer, kMesh._indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
	VkDeviceSize offset[]{ 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &kMesh._verticesBuffer, offset);

	vkCmdDrawIndexed(commandBuffer, kMesh._indices.size(), 1, 0, 0, 0);
}

void	DestroyMesh(const Context& kContext, const LogicalDevice& kLogicalDevice, const Mesh& kMesh)
{
	vkFreeMemory(kLogicalDevice._device, kMesh._indicesMemory, kContext._allocator);
	vkDestroyBuffer(kLogicalDevice._device, kMesh._indicesBuffer, kContext._allocator);

	vkFreeMemory(kLogicalDevice._device, kMesh._verticesMemory, kContext._allocator);
	vkDestroyBuffer(kLogicalDevice._device, kMesh._verticesBuffer, kContext._allocator);
}