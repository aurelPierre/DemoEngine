#pragma once

#include <vulkan/vulkan.h>

#include <array>

#include "Viewport.h"
#include "Texture.h"
#include "Buffer.h"

#include "Wrappers/glm.h"

struct Vertex
{
	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;

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
			return ((hash<Vec3>()(vertex.pos) ^
				(hash<Vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<Vec2>()(vertex.uv) << 1);
		}
	};
}

struct Bindings
{
	enum class Stage
	{
		VERTEX,
		FRAGMENT
	};

	enum class Type
	{
		BUFFER,
		SAMPLER
	};

	uint8_t		_binding	= 0;
	Stage		_stage		= Stage::VERTEX;
	Type		_type		= Type::BUFFER;
	uint8_t		_count		= 1;
};

struct BindingsSet
{
	enum class Scope
	{
		GLOBAL,
		MATERIAL,
		ACTOR
	};

	Scope _scope	= Scope::MATERIAL;

	std::vector<Bindings> _bindings;
};

class Material
{
	struct SetLayout
	{
		BindingsSet				_bindingsSet;
		VkDescriptorSetLayout	_layout			= VK_NULL_HANDLE;
	};

public:
	VkPipelineLayout		_pipelineLayout		= VK_NULL_HANDLE;
	VkPipeline				_pipeline			= VK_NULL_HANDLE;

	std::vector<SetLayout>	_setsLayout;

public:
	Material(const Viewport& kViewport, const std::string kVertextShaderPath,
		const std::string kFragmentShaderPath, const std::vector<BindingsSet>& kSets, 
		const VkCullModeFlagBits kCullMode = VK_CULL_MODE_BACK_BIT);
	~Material();

private:
	void CreateDescriptors(const std::vector<BindingsSet>& kSets);
	void CreatePipeline(const Viewport& kViewport, const std::string kVertextShaderPath,
							const std::string kFragmentShaderPath, const VkCullModeFlagBits kCullMode);
};

class MaterialInstance
{
public:
	const Material* _kMaterial;
	std::vector<VkDescriptorSet> _sets;

public:
	MaterialInstance(const Material& kMaterial, const std::vector<std::vector<void*>>& kData);
	~MaterialInstance();

public:
	void Bind(const CommandBuffer& commandBuffer) const;

	void UpdateSet(const uint8_t kSetIndex, const std::vector<void*>& kData) const;
};

VkShaderModule loadShader(std::string path);
VkPipelineShaderStageCreateInfo createShader(VkShaderModule shaderModule, VkShaderStageFlagBits flags);
