#pragma once

#include <vulkan/vulkan.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>

struct VulkanObject
{
	struct Vertex
	{
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription& getBindingDescription() {
			static VkVertexInputBindingDescription bindingDescription{};

			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2>& getAttributeDescriptions() {
			static std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

	const std::vector<Vertex> vertices = {
		{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
		{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
		{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
	};

	/* Render pipeline */
	VkPipelineLayout						_pipelineLayout = VK_NULL_HANDLE;
	VkPipeline								_pipeline = VK_NULL_HANDLE;

	VkBuffer								_buffer = VK_NULL_HANDLE;
	VkDeviceMemory							_memory = VK_NULL_HANDLE;
};
