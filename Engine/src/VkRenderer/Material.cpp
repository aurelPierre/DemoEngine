#include "VkRenderer/Material.h"

#include "VkRenderer/Context.h"

#include "Core.h"

VkShaderModule loadShader(std::string path)
{
	ASSERT(!path.empty(), "path is empty")

	std::vector<char> code = readFile(path);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;

	VkResult err = vkCreateShaderModule(LogicalDevice::Instance()._device, &createInfo, Context::Instance()._allocator, &shaderModule);
	VK_ASSERT(err, "error when creating shader module");

	return shaderModule;
}

VkPipelineShaderStageCreateInfo createShader(VkShaderModule shaderModule, VkShaderStageFlagBits flags)
{
	VkPipelineShaderStageCreateInfo shaderStageInfo = {};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = flags;

	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName = "main";

	return shaderStageInfo;
}

Material::Material(const Viewport& kViewport, const std::string kVertextShaderPath,
						const std::string kFragmentShaderPath, const std::vector<BindingsSet>& kSets,
						const VkCullModeFlagBits kCullMode)
	: _setsLayout{ kSets.size() }
{
	ASSERT(!kVertextShaderPath.empty(), "kVertextShaderPath is empty")
	ASSERT(!kFragmentShaderPath.empty(), "kFragmentShaderPath is empty")

	CreateDescriptors(kSets);
	CreatePipeline(kViewport, kVertextShaderPath, kFragmentShaderPath, kCullMode);
}

Material::~Material()
{
	for (size_t i = 0; i < _setsLayout.size(); ++i)
		vkDestroyDescriptorSetLayout(LogicalDevice::Instance()._device, _setsLayout[i]._layout, Context::Instance()._allocator);

	vkDestroyPipeline(LogicalDevice::Instance()._device, _pipeline, Context::Instance()._allocator);
	vkDestroyPipelineLayout(LogicalDevice::Instance()._device, _pipelineLayout, Context::Instance()._allocator);
}

void Material::CreateDescriptors(const std::vector<BindingsSet>& kSets)
{
	for (size_t i = 0; i < kSets.size(); ++i)
	{
		_setsLayout[i]._bindingsSet = kSets[i];

		std::vector<VkDescriptorSetLayoutBinding> layoutBinding{ kSets[i]._bindings.size() };
		for (size_t j = 0; j < kSets[i]._bindings.size(); ++j)
		{
			layoutBinding[j].descriptorType = kSets[i]._bindings[j]._type == Bindings::Type::BUFFER ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBinding[j].binding = kSets[i]._bindings[j]._binding;
			layoutBinding[j].stageFlags = kSets[i]._bindings[j]._stage == Bindings::Stage::VERTEX ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBinding[j].descriptorCount = kSets[i]._bindings[j]._count;
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = layoutBinding.size();
		layoutInfo.pBindings = layoutBinding.data();

		VkResult err = vkCreateDescriptorSetLayout(LogicalDevice::Instance()._device, &layoutInfo, Context::Instance()._allocator, &_setsLayout[i]._layout);
		VK_ASSERT(err, "error when creating descriptor set layout");
	}
}

void Material::CreatePipeline(const Viewport& kViewport, const std::string kVertextShaderPath,
								const std::string kFragmentShaderPath, const VkCullModeFlagBits kCullMode)
{
	std::vector<VkDescriptorSetLayout> setLayouts{ _setsLayout.size() };
	for (size_t i = 0; i < _setsLayout.size(); ++i)
		setLayouts[i] = _setsLayout[i]._layout;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = setLayouts.size(); // Optional
	pipelineLayoutInfo.pSetLayouts = setLayouts.data(); // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	VkResult err = vkCreatePipelineLayout(LogicalDevice::Instance()._device, &pipelineLayoutInfo, Context::Instance()._allocator, &_pipelineLayout);
	VK_ASSERT(err, "error when creating pipeline layout");

	// Rendering
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo{};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineInputAssemblyStateCreateInfo.flags = 0;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	pipelineRasterizationStateCreateInfo.cullMode = kCullMode;
	pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineRasterizationStateCreateInfo.flags = 0;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
	pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;
	pipelineColorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	pipelineColorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	pipelineColorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	pipelineColorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	pipelineColorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	pipelineColorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo{};
	pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
	pipelineColorBlendStateCreateInfo.attachmentCount = 1;
	pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
	pipelineColorBlendStateCreateInfo.blendConstants[0] = 1.0f; // Optional
	pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
	pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
	pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineDepthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
	pipelineDepthStencilStateCreateInfo.minDepthBounds = 0.0f; // Optional
	pipelineDepthStencilStateCreateInfo.maxDepthBounds = 1.0f; // Optional
	pipelineDepthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo{};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.scissorCount = 1;
	pipelineViewportStateCreateInfo.flags = 0;

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo{};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineMultisampleStateCreateInfo.flags = 0;

	std::vector<VkDynamicState> dynamicStateEnables = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo{};
	pipelineDynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	pipelineDynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
	pipelineDynamicStateCreateInfo.dynamicStateCount = dynamicStateEnables.size();
	pipelineDynamicStateCreateInfo.flags = 0;

	// Load shaders
	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.layout = _pipelineLayout;
	pipelineCreateInfo.renderPass = kViewport._renderPass;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.basePipelineIndex = -1;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;


	pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;
	pipelineCreateInfo.pDynamicState = &pipelineDynamicStateCreateInfo;

	pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCreateInfo.pStages = shaderStages.data();

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	/*err = vkCreatePipelineCache(kLogicalDevice._device, &pipelineCacheCreateInfo, _contextData._allocator, &_windowData._pipelineCache);
	VK_ASSERT(err);*/

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &Vertex::getBindingDescription();
	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = Vertex::getAttributeDescriptions().size();
	pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = Vertex::getAttributeDescriptions().data();

	pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;

	VkShaderModule vertShaderModule = loadShader(kVertextShaderPath);
	VkShaderModule fragShaderModule = loadShader(kFragmentShaderPath);

	shaderStages[0] = createShader(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = createShader(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT);
	err = vkCreateGraphicsPipelines(LogicalDevice::Instance()._device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, Context::Instance()._allocator, &_pipeline);
	VK_ASSERT(err, "error when creating graphics pipelines");

	vkDestroyShaderModule(LogicalDevice::Instance()._device, vertShaderModule, Context::Instance()._allocator);
	vkDestroyShaderModule(LogicalDevice::Instance()._device, fragShaderModule, Context::Instance()._allocator);
}

MaterialInstance::MaterialInstance(const Material& kMaterial, const std::vector<std::vector<void*>>& kData)
	: _kMaterial{ &kMaterial }, _sets { _kMaterial->_setsLayout.size() }
{
	for (size_t i = 0; i < _kMaterial->_setsLayout.size(); ++i)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = LogicalDevice::Instance()._descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &kMaterial._setsLayout[i]._layout;

		VkResult err = vkAllocateDescriptorSets(LogicalDevice::Instance()._device, &allocInfo, &_sets[i]);
		VK_ASSERT(err, "error when allocating descriptor sets");
	}

	for (size_t i = 0; i < kData.size(); ++i)
		UpdateSet(i, kData[i]);
}

MaterialInstance::~MaterialInstance()
{
	for (size_t i = 0; i < _sets.size(); ++i)
	{
		VkResult err = vkFreeDescriptorSets(LogicalDevice::Instance()._device, LogicalDevice::Instance()._descriptorPool, 1, &_sets[i]);
		VK_ASSERT(err, "error when freeing descriptor sets");
	}
}

void MaterialInstance::Bind(const CommandBuffer& commandBuffer) const
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _kMaterial->_pipeline);

	for (size_t i = 0; i < _sets.size(); ++i)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _kMaterial->_pipelineLayout, i, 1, &_sets[i], 0, nullptr);
}

void MaterialInstance::UpdateSet(const uint8_t kSetIndex, const std::vector<void*>& kData) const
{
	ASSERT(kSetIndex < _kMaterial->_setsLayout.size(), "index is out of size")

	for (size_t j = 0; j < _kMaterial->_setsLayout[kSetIndex]._bindingsSet._bindings.size(); ++j)
	{
		VkWriteDescriptorSet descriptorSet{};
		descriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSet.dstSet = _sets[kSetIndex];
		descriptorSet.dstBinding = _kMaterial->_setsLayout[kSetIndex]._bindingsSet._bindings[j]._binding;
		descriptorSet.descriptorCount = _kMaterial->_setsLayout[kSetIndex]._bindingsSet._bindings[j]._count;

		if (_kMaterial->_setsLayout[kSetIndex]._bindingsSet._bindings[j]._type == Bindings::Type::BUFFER)
		{
			descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			VkDescriptorBufferInfo camBufferInfo = static_cast<Buffer*>(kData[j])->CreateDescriptorInfo();
			descriptorSet.pBufferInfo = &camBufferInfo;
		}
		else
		{
			descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			VkDescriptorImageInfo imageInfo = static_cast<Texture*>(kData[j])->CreateDescriptorInfo();
			descriptorSet.pImageInfo = &imageInfo;
		}

		vkUpdateDescriptorSets(LogicalDevice::Instance()._device, 1, &descriptorSet, 0, nullptr);
	}
}