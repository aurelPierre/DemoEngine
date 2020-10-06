#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <array>

#include "Context.h"
#include "Device.h"
#include "Viewport.h"
#include "Texture.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 color;

	static VkVertexInputBindingDescription& getBindingDescription() {
		static VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3>& getAttributeDescriptions() {
		static std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

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
		attributeDescriptions[2].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color/* && texCoord == other.texCoord*/;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}

struct Material
{
	VkPipelineLayout				_pipelineLayout		= VK_NULL_HANDLE;
	VkPipeline						_pipeline			= VK_NULL_HANDLE;

	// CAMERA
	VkDescriptorSetLayout			_uboLayout			= VK_NULL_HANDLE;
	VkBuffer						_ubo				= VK_NULL_HANDLE;
	VkDeviceMemory					_uboMemory			= VK_NULL_HANDLE;
	VkDescriptorSet					_uboSet				= VK_NULL_HANDLE;
};

VkShaderModule loadShader(const VkDevice kDevice, std::string path);
VkPipelineShaderStageCreateInfo createShader(VkShaderModule shaderModule, VkShaderStageFlagBits flags);

Material CreateMaterial(const Context& kContext, const LogicalDevice& kLogicalDevice, const Device& kDevice,
						const Viewport& kViewport, const std::string kVertextShaderPath,
						const std::string kFragmentShaderPath, const Texture& kTexture);

void	DestroyMaterial(const Context& kContext, const LogicalDevice& kLogicalDevice, const Material& kMaterial);