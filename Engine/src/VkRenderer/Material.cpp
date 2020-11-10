#include "Material.h"

#include "Context.h"

#include "Core.h"

VkShaderModule loadShader(const VkDevice kDevice, std::string path)
{
	ASSERT(!path.empty(), "path is empty")

	std::vector<char> code = readFile(path);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;

	VkResult err = vkCreateShaderModule(kDevice, &createInfo, nullptr, &shaderModule);
	check_vk_result(err);

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

Material::Material(const Device& kDevice, const Viewport& kViewport, const std::string kVertextShaderPath,
						const std::string kFragmentShaderPath, const std::vector<Texture*>& kTextures)
{
	ASSERT(!kVertextShaderPath.empty(), "kVertextShaderPath is empty")
	ASSERT(!kFragmentShaderPath.empty(), "kFragmentShaderPath is empty")

	Buffer ubo(kDevice, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	_ubo = std::move(ubo);
	
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBinding{ 1 + kTextures.size() };
		layoutBinding[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding[0].binding = 0;
		layoutBinding[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		layoutBinding[0].descriptorCount = 1;

		for (size_t i = 0; i < kTextures.size(); ++i)
		{
			layoutBinding[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			layoutBinding[i + 1].binding = i + 1;
			layoutBinding[i + 1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			layoutBinding[i + 1].descriptorCount = 1;
		}


		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = layoutBinding.size();
		layoutInfo.pBindings = layoutBinding.data();

		VkResult err = vkCreateDescriptorSetLayout(LogicalDevice::Instance()._device, &layoutInfo,
													Context::Instance()._allocator, &_uboLayout);
		check_vk_result(err);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = LogicalDevice::Instance()._descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_uboLayout;
		err = vkAllocateDescriptorSets(LogicalDevice::Instance()._device, &allocInfo, &_uboSet);
		check_vk_result(err);

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = _ubo._buffer;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		std::vector<VkWriteDescriptorSet> descriptorWrites{ 1 + kTextures.size() };
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = _uboSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		
		std::vector<VkDescriptorImageInfo> imagesInfo{ kTextures.size() };
		for (size_t i = 0; i < kTextures.size(); ++i)
		{
			imagesInfo[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imagesInfo[i].imageView = kTextures[i]->_image._view;
			imagesInfo[i].sampler = kTextures[i]->_sampler;

			descriptorWrites[i + 1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[i + 1].dstSet = _uboSet;
			descriptorWrites[i + 1].dstBinding = i + 1;
			descriptorWrites[i + 1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[i + 1].descriptorCount = 1;
			descriptorWrites[i + 1].pImageInfo = &imagesInfo[i];
		}
		
		vkUpdateDescriptorSets(LogicalDevice::Instance()._device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}

	/******************************************************************************************************/

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1; // Optional
	pipelineLayoutInfo.pSetLayouts = &_uboLayout; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	VkResult err = vkCreatePipelineLayout(LogicalDevice::Instance()._device, &pipelineLayoutInfo, Context::Instance()._allocator, &_pipelineLayout);
	check_vk_result(err);

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
	check_vk_result(err);*/

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &Vertex::getBindingDescription();
	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = Vertex::getAttributeDescriptions().size();
	pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = Vertex::getAttributeDescriptions().data();

	pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;

	VkShaderModule vertShaderModule = loadShader(LogicalDevice::Instance()._device, kVertextShaderPath);
	VkShaderModule fragShaderModule = loadShader(LogicalDevice::Instance()._device, kFragmentShaderPath);

	shaderStages[0] = createShader(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = createShader(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT);
	err = vkCreateGraphicsPipelines(LogicalDevice::Instance()._device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, Context::Instance()._allocator, &_pipeline);
	check_vk_result(err);


	vkDestroyShaderModule(LogicalDevice::Instance()._device, vertShaderModule, Context::Instance()._allocator);
	vkDestroyShaderModule(LogicalDevice::Instance()._device, fragShaderModule, Context::Instance()._allocator);
}

Material::~Material()
{
	vkFreeDescriptorSets(LogicalDevice::Instance()._device, LogicalDevice::Instance()._descriptorPool, 1, &_uboSet);
	vkDestroyDescriptorSetLayout(LogicalDevice::Instance()._device, _uboLayout, Context::Instance()._allocator);

	vkDestroyPipeline(LogicalDevice::Instance()._device, _pipeline, Context::Instance()._allocator);
	vkDestroyPipelineLayout(LogicalDevice::Instance()._device, _pipelineLayout, Context::Instance()._allocator);
}