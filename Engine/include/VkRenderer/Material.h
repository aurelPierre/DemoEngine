#pragma once

#include <vulkan/vulkan.h>

#include <array>

#include "Viewport.h"
#include "Texture.h"
#include "Buffer.h"

#include "Wrappers/glm.h"

struct Vertex
{
	enum DataFlag
	{
		POSITION = 1 << 0,
		UV = 1 << 1,
		NORMAL = 1 << 2,
		TANGENT = 1 << 3
	};

	Vec3 pos;
	Vec2 uv;
	Vec3 normal;
	Vec3 tangent;

	static VkVertexInputBindingDescription getBindingDescription(int kDataFlags) {
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(int kDataFlags) {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		uint8_t locationNb = 0;
		if (kDataFlags & DataFlag::POSITION)
		{
			attributeDescriptions.push_back({ locationNb, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) });
			locationNb++;
		}

		if (kDataFlags & DataFlag::UV)
		{
			attributeDescriptions.push_back({ locationNb, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });
			locationNb++;
		}

		if (kDataFlags & DataFlag::NORMAL)
		{
			attributeDescriptions.push_back({ locationNb, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
			locationNb++;
		}

		if (kDataFlags & DataFlag::TANGENT)
		{
			attributeDescriptions.push_back({ locationNb, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent) });
			locationNb++;
		}

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
		const VkCullModeFlagBits kCullMode = VK_CULL_MODE_BACK_BIT,
		const int kVertexDataFlags = Vertex::POSITION | Vertex::UV | Vertex::NORMAL | Vertex::TANGENT,
		const bool kWireframe = false);
	~Material();

private:
	void CreateDescriptors(const std::vector<BindingsSet>& kSets);
	void CreatePipeline(const Viewport& kViewport, const std::string kVertextShaderPath,
							const std::string kFragmentShaderPath, const VkCullModeFlagBits kCullMode,
							const int kVertexDataFlags = Vertex::POSITION | Vertex::UV | Vertex::NORMAL | Vertex::TANGENT,
							const bool kWireframe = false);
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
