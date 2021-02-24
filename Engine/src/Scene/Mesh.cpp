#include "Scene/Mesh.h"

#include <unordered_map>

#include "tiny_obj_loader.h"
#include "Core.h"

Mesh::Mesh(const std::string kPath)
{
	ASSERT(!kPath.empty(), "kPath is empty")

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	bool loaded = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, kPath.c_str(), nullptr, false);
	ASSERT(loaded, "Load obj " + kPath + "failed. Warn: " + warn + "; Err: " + err)

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto& shape : shapes) 
	{
		size_t indexOffset = 0;
		for (size_t i = 0; i < shape.mesh.num_face_vertices.size(); ++i)
		{
			size_t fv = shape.mesh.num_face_vertices[i];
			if(fv > 3)
				LOG(ez::WARNING, "Mesh " + kPath + " have a face with more than 3 vertices, tangent might be wrong")

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
		Buffer indicesBuf(sizeof(_indices[0]) * _indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		_indicesBuffer = std::move(indicesBuf);
		_indicesBuffer.Map(_indices.data(), sizeof(_indices[0]) * _indices.size());
	}

	{
		Buffer verticesBuffer(sizeof(_vertices[0]) * _vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		_verticesBuffer = std::move(verticesBuffer);
		_verticesBuffer.Map(_vertices.data(), sizeof(_vertices[0]) * _vertices.size());
	}
}

void Mesh::Draw(const CommandBuffer& commandBuffer) const
{
	vkCmdBindIndexBuffer(commandBuffer, _indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
	VkDeviceSize offset[]{ 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &_verticesBuffer._buffer, offset);

	vkCmdDrawIndexed(commandBuffer, _indices.size(), 1, 0, 0, 0);
}
