#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>

#include "Viewport.h"
#include "Texture.h"
#include "Buffer.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 normal;
	glm::vec3 tangent;

	static VkVertexInputBindingDescription& getBindingDescription() {
		static VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4>& getAttributeDescriptions() {
		static std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, uv);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, tangent);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && normal == other.normal && uv == other.uv;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}

class Material
{
public:
	VkPipelineLayout				_pipelineLayout		= VK_NULL_HANDLE;
	VkPipeline						_pipeline			= VK_NULL_HANDLE;

	VkDescriptorSetLayout			_globalLayout		= VK_NULL_HANDLE;
	VkDescriptorSet					_globalSet			= VK_NULL_HANDLE;

	VkDescriptorSetLayout			_materialLayout		= VK_NULL_HANDLE;
	VkDescriptorSet					_materialSet		= VK_NULL_HANDLE;

	VkDescriptorSetLayout			_objectsLayout		= VK_NULL_HANDLE;
	VkDescriptorSet					_objectsSet			= VK_NULL_HANDLE;

public:
	Material(const Viewport& kViewport, const std::string kVertextShaderPath,
		const std::string kFragmentShaderPath, const size_t kTextureSize);
	~Material();

private:
	void CreateDescriptors(const size_t kTextureSize);
	void CreatePipeline(const Viewport& kViewport, const std::string kVertextShaderPath,
							const std::string kFragmentShaderPath);

public:
	void UpdateDescriptors(const Buffer& kCamera, const Buffer& kLight, const std::vector<Texture*>& kTextures, const Buffer& kModel) const;
};

VkShaderModule loadShader(std::string path);
VkPipelineShaderStageCreateInfo createShader(VkShaderModule shaderModule, VkShaderStageFlagBits flags);
