#include "VkRenderer/Material.h"

#include "VkRenderer/Context.h"

#include "VkRenderer/Core.h"

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
						const std::string kFragmentShaderPath, const size_t kTextureSize)
{
	ASSERT(!kVertextShaderPath.empty(), "kVertextShaderPath is empty")
	ASSERT(!kFragmentShaderPath.empty(), "kFragmentShaderPath is empty")

	CreateDescriptors(kTextureSize);
	CreatePipeline(kViewport, kVertextShaderPath, kFragmentShaderPath);
}

Material::~Material()
{
	vkFreeDescriptorSets(LogicalDevice::Instance()._device, LogicalDevice::Instance()._descriptorPool, 1, &_globalSet);
	vkDestroyDescriptorSetLayout(LogicalDevice::Instance()._device, _globalLayout, Context::Instance()._allocator);

	vkFreeDescriptorSets(LogicalDevice::Instance()._device, LogicalDevice::Instance()._descriptorPool, 1, &_materialSet);
	vkDestroyDescriptorSetLayout(LogicalDevice::Instance()._device, _materialLayout, Context::Instance()._allocator);

	vkFreeDescriptorSets(LogicalDevice::Instance()._device, LogicalDevice::Instance()._descriptorPool, 1, &_objectsSet);
	vkDestroyDescriptorSetLayout(LogicalDevice::Instance()._device, _objectsLayout, Context::Instance()._allocator);

	vkDestroyPipeline(LogicalDevice::Instance()._device, _pipeline, Context::Instance()._allocator);
	vkDestroyPipelineLayout(LogicalDevice::Instance()._device, _pipelineLayout, Context::Instance()._allocator);
}

void Material::CreateDescriptors(const size_t kTextureSize)
{
	// GLOBAL
	{
		std::vector<VkDescriptorSetLayoutBinding> gloLayoutBinding{ 2 };
		gloLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		gloLayoutBinding[0].binding = 0;
		gloLayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		gloLayoutBinding[0].descriptorCount = 1;

		gloLayoutBinding[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		gloLayoutBinding[1].binding = 1;
		gloLayoutBinding[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		gloLayoutBinding[1].descriptorCount = 1;

		VkDescriptorSetLayoutCreateInfo gloLayoutInfo{};
		gloLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		gloLayoutInfo.bindingCount = gloLayoutBinding.size();
		gloLayoutInfo.pBindings = gloLayoutBinding.data();

		VkResult err = vkCreateDescriptorSetLayout(LogicalDevice::Instance()._device, &gloLayoutInfo,
			Context::Instance()._allocator, &_globalLayout);
		VK_ASSERT(err, "error when creating descriptor set layout");

		VkDescriptorSetAllocateInfo gloAllocInfo{};
		gloAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		gloAllocInfo.descriptorPool = LogicalDevice::Instance()._descriptorPool;
		gloAllocInfo.descriptorSetCount = 1;
		gloAllocInfo.pSetLayouts = &_globalLayout;

		err = vkAllocateDescriptorSets(LogicalDevice::Instance()._device, &gloAllocInfo, &_globalSet);
		VK_ASSERT(err, "error when allocating descriptor sets");
	}
	
	// MATERIAL
	{
		std::vector<VkDescriptorSetLayoutBinding> matlayoutBinding{ kTextureSize };
		for (size_t i = 0; i < kTextureSize; ++i)
		{
			matlayoutBinding[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			matlayoutBinding[i].binding = i;
			matlayoutBinding[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			matlayoutBinding[i].descriptorCount = 1;
		}

		VkDescriptorSetLayoutCreateInfo matLayoutInfo{};
		matLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		matLayoutInfo.bindingCount = matlayoutBinding.size();
		matLayoutInfo.pBindings = matlayoutBinding.data();

		VkResult err = vkCreateDescriptorSetLayout(LogicalDevice::Instance()._device, &matLayoutInfo,
			Context::Instance()._allocator, &_materialLayout);
		VK_ASSERT(err, "error when creating descriptor set layout");

		VkDescriptorSetAllocateInfo matAllocInfo{};
		matAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		matAllocInfo.descriptorPool = LogicalDevice::Instance()._descriptorPool;
		matAllocInfo.descriptorSetCount = 1;
		matAllocInfo.pSetLayouts = &_materialLayout;

		err = vkAllocateDescriptorSets(LogicalDevice::Instance()._device, &matAllocInfo, &_materialSet);
		VK_ASSERT(err, "error when allocating descriptor sets");
	}
	
	// OBJECTS
	{
		std::vector<VkDescriptorSetLayoutBinding> objLayoutBinding{ 1 };
		objLayoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		objLayoutBinding[0].binding = 0;
		objLayoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		objLayoutBinding[0].descriptorCount = 1;

		VkDescriptorSetLayoutCreateInfo objLayoutInfo{};
		objLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		objLayoutInfo.bindingCount = objLayoutBinding.size();
		objLayoutInfo.pBindings = objLayoutBinding.data();

		VkResult err = vkCreateDescriptorSetLayout(LogicalDevice::Instance()._device, &objLayoutInfo,
			Context::Instance()._allocator, &_objectsLayout);

		VkDescriptorSetAllocateInfo objAllocInfo{};
		objAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		objAllocInfo.descriptorPool = LogicalDevice::Instance()._descriptorPool;
		objAllocInfo.descriptorSetCount = 1;
		objAllocInfo.pSetLayouts = &_objectsLayout;

		err = vkAllocateDescriptorSets(LogicalDevice::Instance()._device, &objAllocInfo, &_objectsSet);
		VK_ASSERT(err, "allocating descriptor sets");
	}
}

void Material::CreatePipeline(const Viewport& kViewport, const std::string kVertextShaderPath,
								const std::string kFragmentShaderPath)
{
	VkDescriptorSetLayout _setLayouts[]{ _globalLayout, _materialLayout, _objectsLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 3; // Optional
	pipelineLayoutInfo.pSetLayouts = _setLayouts; // Optional
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
	pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
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
	pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
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

void Material::UpdateDescriptors(const Buffer& kCamera, const Buffer& kLight, const std::vector<Texture*>& kTextures, const Buffer& kModel) const
{
	std::vector<VkWriteDescriptorSet> descriptorWrites{ 2 + kTextures.size() + 1 };

	// GLOBAL
	VkDescriptorBufferInfo camBufferInfo = kCamera.CreateDescriptorInfo();
	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = _globalSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &camBufferInfo;

	VkDescriptorBufferInfo lightBufferInfo = kLight.CreateDescriptorInfo();
	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = _globalSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &lightBufferInfo;

	// MATERIAL
	std::vector<VkDescriptorImageInfo> imagesInfo{ kTextures.size() };
	for (size_t i = 0; i < kTextures.size(); ++i)
	{
		imagesInfo[i] = kTextures[i]->CreateDescriptorInfo();

		descriptorWrites[i + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i + 2].dstSet = _materialSet;
		descriptorWrites[i + 2].dstBinding = i;
		descriptorWrites[i + 2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[i + 2].descriptorCount = 1;
		descriptorWrites[i + 2].pImageInfo = &imagesInfo[i];
	}

	// OBJECTS
	VkDescriptorBufferInfo objBufferInfo = kModel.CreateDescriptorInfo();
	descriptorWrites[kTextures.size() + 2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[kTextures.size() + 2].dstSet = _objectsSet;
	descriptorWrites[kTextures.size() + 2].dstBinding = 0;
	descriptorWrites[kTextures.size() + 2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[kTextures.size() + 2].descriptorCount = 1;
	descriptorWrites[kTextures.size() + 2].pBufferInfo = &objBufferInfo;
	

	vkUpdateDescriptorSets(LogicalDevice::Instance()._device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}