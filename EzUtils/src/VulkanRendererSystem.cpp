#include "VulkanRendererSystem.h"

#include "Utils.h"

#include <imgui_impl_vulkan.h>

#include "GLFWWindowSystem.h"
#include "LogSystem.h"

#include <array>
#include <map>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Core.h"

void VulkanRendererSystem::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "EzEditor";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "EzEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t layerCount;
	VkResult err = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	check_vk_result(err);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	err = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	check_vk_result(err);

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
	}

	createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
	createInfo.ppEnabledLayerNames = validationLayers.data();

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	err = vkCreateInstance(&createInfo, _contextData._allocator, &_contextData._instance);
	check_vk_result(err);
}

void VulkanRendererSystem::SetupDebug()
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_contextData._instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		VkResult err = func(_contextData._instance, &createInfo, _contextData._allocator, &_contextData._debugMessenger);
		check_vk_result(err);
	}
	else {
		throw std::runtime_error("validation layers requested, but not available!");
	}
}

int	VulkanRendererSystem::RateDeviceSuitability(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	int score = 0;

	// Discrete GPUs have a significant performance advantage
	if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		score += 1000;
	}

	// Maximum possible size of textures affects graphics quality
	score += deviceProperties.limits.maxImageDimension2D;

	// Application can't function without geometry shaders
	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	return score;
}


void VulkanRendererSystem::PickPhysicalDevice()
{
	uint32_t gpu_count;
	VkResult err = vkEnumeratePhysicalDevices(_contextData._instance, &gpu_count, NULL);
	check_vk_result(err);
	IM_ASSERT(gpu_count > 0);

	std::vector<VkPhysicalDevice> physicalDevices(gpu_count);
	err = vkEnumeratePhysicalDevices(_contextData._instance, &gpu_count, physicalDevices.data());
	check_vk_result(err);

	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : physicalDevices) {
		int score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0) {
		_deviceData._physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(_deviceData._physicalDevice, &_deviceData._properties);
	vkGetPhysicalDeviceFeatures(_deviceData._physicalDevice, &_deviceData._features);
	vkGetPhysicalDeviceMemoryProperties(_deviceData._physicalDevice, &_deviceData._memoryProperties);

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(_deviceData._physicalDevice, &queueFamilyCount, nullptr);
	assert(queueFamilyCount > 0);
	_deviceData._queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_deviceData._physicalDevice, &queueFamilyCount, _deviceData._queueFamilyProperties.data());

	// Get list of supported extensions
	uint32_t extCount = 0;
	err = vkEnumerateDeviceExtensionProperties(_deviceData._physicalDevice, nullptr, &extCount, nullptr);
	check_vk_result(err);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);

		err = vkEnumerateDeviceExtensionProperties(_deviceData._physicalDevice, nullptr, &extCount, &extensions.front());
		check_vk_result(err);

		for (VkExtensionProperties& ext : extensions)
			_deviceData._supportedExtensions.push_back(ext.extensionName);
	}

	_deviceData._enabledFeatures = _deviceData._features;
}

void VulkanRendererSystem::CreateLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Get queue family indices for the requested queue family types
	// Note that the indices may overlap depending on the implementation

	const float defaultQueuePriority{ 0.f };

	for (uint32_t i = 0; i < static_cast<uint32_t>(_deviceData._queueFamilyProperties.size()); i++)
	{
		if (_deviceData._queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			_deviceData._queueFamilyIndices._graphics = i;
		else if (_deviceData._queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			_deviceData._queueFamilyIndices._compute = i;
		else if (_deviceData._queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			_deviceData._queueFamilyIndices._transfer = i;
	}

	// Graphics queue
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = _deviceData._queueFamilyIndices._graphics;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	// Dedicated compute queue
	{
		if (_deviceData._queueFamilyIndices._graphics != _deviceData._queueFamilyIndices._compute)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = _deviceData._queueFamilyIndices._compute;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}

	// Dedicated transfer queue
	{
		if ((_deviceData._queueFamilyIndices._transfer != _deviceData._queueFamilyIndices._graphics)
			&& (_deviceData._queueFamilyIndices._transfer != _deviceData._queueFamilyIndices._compute))
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = _deviceData._queueFamilyIndices._transfer;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}

	// Create the logical device representation
	std::vector<const char*> deviceExtensions;
	deviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	/*for (const auto& extension : _deviceData._supportedExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}*/

	// Enable the debug marker extension if it is present (likely meaning a debugging tool is present)
	//deviceExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME); // dunno

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.pEnabledFeatures = &_deviceData._enabledFeatures;

	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// to ensure older implementation compatibility
	deviceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();

	VkResult err = vkCreateDevice(_deviceData._physicalDevice, &deviceCreateInfo, _contextData._allocator, &_deviceData._device);
	check_vk_result(err);

	vkGetDeviceQueue(_deviceData._device, _deviceData._queueFamilyIndices._graphics, 0, &_deviceData._graphicsQueue);
	vkGetDeviceQueue(_deviceData._device, _deviceData._queueFamilyIndices._compute, 0, &_deviceData._computeQueue);
	vkGetDeviceQueue(_deviceData._device, _deviceData._queueFamilyIndices._transfer, 0, &_deviceData._transferQueue);

	// Descriptor pool (paste from imgui exemple)
	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	err = vkCreateDescriptorPool(_deviceData._device, &pool_info, _contextData._allocator, &_deviceData._descriptorPool);
	check_vk_result(err);
}

void VulkanRendererSystem::CreateSurface(const GLFWWindowData* windowData)
{
	VkResult err = glfwCreateWindowSurface(_contextData._instance, windowData->_window, _contextData._allocator, &_windowData._surface);
	check_vk_result(err);

	VkBool32 res;
	err = vkGetPhysicalDeviceSurfaceSupportKHR(_deviceData._physicalDevice, _deviceData._queueFamilyIndices._graphics, _windowData._surface, &res);
	check_vk_result(err);
	if (res != VK_TRUE)
	{
		LOG(ez::ERROR, std::string("Error no WSI support on physical device 0"))
			exit(-1);
	}

	// Get list of supported surface formats
	uint32_t formatCount;
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(_deviceData._physicalDevice, _windowData._surface, &formatCount, NULL);
	check_vk_result(err);
	assert(formatCount > 0);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	err = vkGetPhysicalDeviceSurfaceFormatsKHR(_deviceData._physicalDevice, _windowData._surface, &formatCount, surfaceFormats.data());
	check_vk_result(err);

	// If the surface format list only includes one entry with VK_FORMAT_UNDEFINED,
	// there is no preferered format, so we assume VK_FORMAT_B8G8R8A8_UNORM
	if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
	{
		_windowData._colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
		_windowData._colorSpace = surfaceFormats[0].colorSpace;
	}
	else
	{
		// iterate over the list of available surface format and
		// check for the presence of VK_FORMAT_B8G8R8A8_UNORM
		bool found_B8G8R8A8_UNORM = false;
		for (VkSurfaceFormatKHR& surfaceFormat : surfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				_windowData._colorFormat = surfaceFormat.format;
				_windowData._colorSpace = surfaceFormat.colorSpace;
				found_B8G8R8A8_UNORM = true;
				break;
			}
		}

		// in case VK_FORMAT_B8G8R8A8_UNORM is not available
		// select the first available color format
		if (!found_B8G8R8A8_UNORM)
		{
			_windowData._colorFormat = surfaceFormats[0].format;
			_windowData._colorSpace = surfaceFormats[0].colorSpace;
		}
	}
}

void VulkanRendererSystem::CreateSwapchain(const GLFWWindowData* windowData)
{
	VkSurfaceCapabilitiesKHR surfCaps;
	VkResult err = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_deviceData._physicalDevice, _windowData._surface, &surfCaps);
	check_vk_result(err);
	// Get available present modes
	uint32_t presentModeCount;
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(_deviceData._physicalDevice, _windowData._surface, &presentModeCount, NULL);
	check_vk_result(err);
	assert(presentModeCount > 0);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);
	err = vkGetPhysicalDeviceSurfacePresentModesKHR(_deviceData._physicalDevice, _windowData._surface, &presentModeCount, presentModes.data());
	check_vk_result(err);

	int w, h;
	glfwGetFramebufferSize(windowData->_window, &w, &h);

	VkExtent2D swapchainExtent = {};
	// If width (and height) equals the special value 0xFFFFFFFF, the size of the surface will be set by the swapchain
	if (surfCaps.currentExtent.width == (uint32_t)-1)
	{
		// If the surface size is undefined, the size is set to
		// the size of the images requested.
		swapchainExtent.width = w;
		swapchainExtent.height = h;
	}
	else
	{
		// TODO check wtf
		// If the surface size is defined, the swap chain size must match
		swapchainExtent = surfCaps.currentExtent;
		//*width = surfCaps.currentExtent.width;
		//*height = surfCaps.currentExtent.height;
	}

	// Select a present mode for the swapchain

	// The VK_PRESENT_MODE_FIFO_KHR mode must always be present as per spec
	// This mode waits for the vertical blank ("v-sync")
	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// Determine the number of images
	uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
	{
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	// Find the transformation of the surface
	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		// We prefer a non-rotated transform
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfCaps.currentTransform;
	}

	// Find a supported composite alpha format (not all devices support alpha opaque)
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	// Simply select the first composite alpha format available
	std::vector<VkCompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};
	for (auto& compositeAlphaFlag : compositeAlphaFlags) {
		if (surfCaps.supportedCompositeAlpha & compositeAlphaFlag) {
			compositeAlpha = compositeAlphaFlag;
			break;
		};
	}

	VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;// _windowData._swapchain;

	VkSwapchainCreateInfoKHR swapchainCI = {};
	swapchainCI.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCI.pNext = NULL;
	swapchainCI.surface = _windowData._surface;
	swapchainCI.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCI.imageFormat = _windowData._colorFormat;
	swapchainCI.imageColorSpace = _windowData._colorSpace;
	swapchainCI.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchainCI.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCI.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCI.imageArrayLayers = 1;
	swapchainCI.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCI.queueFamilyIndexCount = 0;
	swapchainCI.pQueueFamilyIndices = NULL;
	swapchainCI.presentMode = swapchainPresentMode;
	//swapchainCI.oldSwapchain = oldSwapchain;
	// Setting clipped to VK_TRUE allows the implementation to discard rendering outside of the surface area
	swapchainCI.clipped = VK_FALSE;
	swapchainCI.compositeAlpha = compositeAlpha;

	// Enable transfer source on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	// Enable transfer destination on swap chain images if supported
	if (surfCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
		swapchainCI.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}

	err = vkCreateSwapchainKHR(_deviceData._device, &swapchainCI, _contextData._allocator, &_windowData._swapchain);
	check_vk_result(err);

	// If an existing swap chain is re-created, destroy the old swap chain
	// This also cleans up all the presentable images
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < _windowData._imageCount; i++)
		{
			vkDestroyImageView(_deviceData._device, _windowData._frames[i]._imageView, nullptr);
		}
		vkDestroySwapchainKHR(_deviceData._device, oldSwapchain, nullptr);
	}
}

void VulkanRendererSystem::CreateRenderPass()
{
	// Final pass ie. Editor
	VkAttachmentDescription attachment = {};
	attachment.format = _windowData._colorFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference final_attachment = {};
	final_attachment.attachment = 0;
	final_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &final_attachment;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	info.attachmentCount = 1;
	info.pAttachments = &attachment;
	info.subpassCount = 1;
	info.pSubpasses = &subpass;
	info.dependencyCount = 1;
	info.pDependencies = &dependency;

	VkResult err = vkCreateRenderPass(_deviceData._device, &info, _contextData._allocator, &_windowData._renderPass);
	check_vk_result(err);
}

void VulkanRendererSystem::CreateGraphicsPipeline()
{
	/*VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0; // Optional
	pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

	VkResult err = vkCreatePipelineLayout(_deviceData._device, &pipelineLayoutInfo, _contextData._allocator, &_objects._pipelineLayout);
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
	pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineRasterizationStateCreateInfo.flags = 0;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState{};
	pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState.blendEnable = VK_TRUE;
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
	pipelineColorBlendStateCreateInfo.blendConstants[0] = 0.0f; // Optional
	pipelineColorBlendStateCreateInfo.blendConstants[1] = 0.0f; // Optional
	pipelineColorBlendStateCreateInfo.blendConstants[2] = 0.0f; // Optional
	pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f; // Optional

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo{};
	pipelineDepthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	pipelineDepthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	pipelineDepthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;



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
	pipelineCreateInfo.layout = _objects._pipelineLayout;
	pipelineCreateInfo.renderPass = _windowData._renderPass;
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

	std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
		VkVertexInputBindingDescription{0, vertexLayout.stride(), VK_VERTEX_INPUT_RATE_VERTEX }
	};
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
		VkVertexInputAttributeDescription{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},					// Location 0: Position
		VkVertexInputAttributeDescription{0, 1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3},	// Location 1: Normal
		VkVertexInputAttributeDescription{0, 2, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 6},	// Location 2: Color
	};

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	err = vkCreatePipelineCache(_deviceData._device, &pipelineCacheCreateInfo, _contextData._allocator, &_windowData._pipelineCache);
	check_vk_result(err);

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &VulkanObject::Vertex::getBindingDescription();
	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
	pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = VulkanObject::Vertex::getAttributeDescriptions().data();

	pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;

	VkShaderModule vertShaderModule = loadShader("D:/Personal project/EzUtils/shaders/bin/shader.vert.spv");
	VkShaderModule fragShaderModule = loadShader("D:/Personal project/EzUtils/shaders/bin/shader.frag.spv");

	shaderStages[0] = createShader(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT);
	shaderStages[1] = createShader(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT);
	err = vkCreateGraphicsPipelines(_deviceData._device, _windowData._pipelineCache, 1, &pipelineCreateInfo, _contextData._allocator, &_objects._pipeline);
	check_vk_result(err);


	vkDestroyShaderModule(_deviceData._device, vertShaderModule, _contextData._allocator);
	vkDestroyShaderModule(_deviceData._device, fragShaderModule, _contextData._allocator);*/
}

void VulkanRendererSystem::CreateImageViews()
{
	VkResult err = vkGetSwapchainImagesKHR(_deviceData._device, _windowData._swapchain, &_windowData._imageCount, NULL);
	check_vk_result(err);

	// Get the swap chain images
	std::vector<VkImage> images(_windowData._imageCount);
	err = vkGetSwapchainImagesKHR(_deviceData._device, _windowData._swapchain, &_windowData._imageCount, images.data());
	check_vk_result(err);

	_windowData._frames.resize(_windowData._imageCount);
	// Get the swap chain buffers containing the image and imageview
	for (uint32_t i = 0; i < _windowData._imageCount; i++)
	{
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = _windowData._colorFormat;
		colorAttachmentView.components = {
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;

		_windowData._frames[i]._image = images[i];

		colorAttachmentView.image = _windowData._frames[i]._image;

		err = vkCreateImageView(_deviceData._device, &colorAttachmentView, _contextData._allocator, &_windowData._frames[i]._imageView);
		check_vk_result(err);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;

		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy = 16.0f;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

		samplerInfo.unnormalizedCoordinates = VK_FALSE;

		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		if (vkCreateSampler(_deviceData._device, &samplerInfo, _contextData._allocator, &_windowData._frames[i]._sampler) != VK_SUCCESS) {
			throw std::runtime_error("failed to create texture sampler!");
		}
	}
}

void VulkanRendererSystem::CreateFramebuffers()
{
	VkResult err;

	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = _deviceData._queueFamilyIndices._graphics;
		err = vkCreateCommandPool(_deviceData._device, &info, _contextData._allocator, &_windowData._commandPool);
		check_vk_result(err);
	}

	for (uint32_t i = 0; i < _windowData._frames.size(); i++)
	{
		VkImageView attachment[1];
		attachment[0] = _windowData._frames[i]._imageView;

		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = _windowData._renderPass;
		info.attachmentCount = 1;
		info.pAttachments = attachment;
		info.width = _windowData._windowData->_width;
		info.height = _windowData._windowData->_height;
		info.layers = 1;

		{
			err = vkCreateFramebuffer(_deviceData._device, &info, _contextData._allocator, &_windowData._frames[i]._framebuffer);
			check_vk_result(err);
		}

		{
			VkCommandBufferAllocateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			info.commandPool = _windowData._commandPool;
			info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			info.commandBufferCount = 1;
			err = vkAllocateCommandBuffers(_deviceData._device, &info, &_windowData._frames[i]._commandBuffer);
			check_vk_result(err);
		}
		{
			VkFenceCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
			err = vkCreateFence(_deviceData._device, &info, _contextData._allocator, &_windowData._frames[i]._fence);
			check_vk_result(err);
		}
		{
			VkSemaphoreCreateInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
			err = vkCreateSemaphore(_deviceData._device, &info, _contextData._allocator, &_windowData._frames[i]._presentComplete);
			check_vk_result(err);
			err = vkCreateSemaphore(_deviceData._device, &info, _contextData._allocator, &_windowData._frames[i]._renderComplete);
			check_vk_result(err);
		}
	}
}

VkShaderModule VulkanRendererSystem::loadShader(std::string path)
{
	std::vector<char> code = readFile(path);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;

	VkResult err = vkCreateShaderModule(_deviceData._device, &createInfo, nullptr, &shaderModule);
	check_vk_result(err);

	return shaderModule;
}

VkPipelineShaderStageCreateInfo VulkanRendererSystem::createShader(VkShaderModule shaderModule, VkShaderStageFlagBits flags)
{
	VkPipelineShaderStageCreateInfo shaderStageInfo = {};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = flags;

	shaderStageInfo.module = shaderModule;
	shaderStageInfo.pName = "main";

	return shaderStageInfo;
}

VulkanContext* VulkanRendererSystem::CreateVulkanContext()
{
	CreateInstance();
	SetupDebug();
	return &_contextData;
}

void VulkanRendererSystem::DeleteVulkanContext()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_contextData._instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(_contextData._instance, _contextData._debugMessenger, _contextData._allocator);
	}

	vkDestroyInstance(_contextData._instance, _contextData._allocator);
}

VulkanDevice* VulkanRendererSystem::CreateVulkanDevice()
{
	PickPhysicalDevice();
	CreateLogicalDevice();
	return &_deviceData;
}

void VulkanRendererSystem::DeleteVulkanDevice()
{
	vkDestroyDescriptorPool(_deviceData._device, _deviceData._descriptorPool, _contextData._allocator);

	vkDestroyDevice(_deviceData._device, _contextData._allocator);
}

VulkanWindow* VulkanRendererSystem::CreateVulkanWindow(const GLFWWindowData* windowData)
{
	_windowData._windowData = windowData;

	CreateSurface(windowData);
	CreateSwapchain(windowData);
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateFramebuffers();

	/*VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(_objects.vertices[0]) * _objects.vertices.size();
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(_deviceData._device, &bufferInfo, nullptr, &_objects._buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(_deviceData._device, _objects._buffer, &memRequirements);

	uint32_t type = UINT32_MAX;
	for (uint32_t i = 0; i < _deviceData._memoryProperties.memoryTypeCount; i++) {
		if ((memRequirements.memoryTypeBits & (1 << i)) && (_deviceData._memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			type = i;
		}
	}
	if (type == UINT32_MAX)
		throw;

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = type;

	if (vkAllocateMemory(_deviceData._device, &allocInfo, nullptr, &_objects._memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(_deviceData._device, _objects._buffer, _objects._memory, 0);*/

	return &_windowData;
}

void VulkanRendererSystem::DeleteVulkanWindow()
{
	vkDeviceWaitIdle(_deviceData._device);

	/*vkFreeMemory(_deviceData._device, _objects._memory, nullptr);
	vkDestroyBuffer(_deviceData._device, _objects._buffer, nullptr);*/

	// Frames
	for (uint32_t i = 0; i < _windowData._frames.size(); i++)
	{
		vkDestroySemaphore(_deviceData._device, _windowData._frames[i]._presentComplete, _contextData._allocator);
		vkDestroySemaphore(_deviceData._device, _windowData._frames[i]._renderComplete, _contextData._allocator);

		vkDestroyFence(_deviceData._device, _windowData._frames[i]._fence, _contextData._allocator);
		vkDestroyFramebuffer(_deviceData._device, _windowData._frames[i]._framebuffer, _contextData._allocator);
		vkDestroyImageView(_deviceData._device, _windowData._frames[i]._imageView, _contextData._allocator);
		vkDestroySampler(_deviceData._device, _windowData._frames[i]._sampler, nullptr);
	}

	vkDestroyCommandPool(_deviceData._device, _windowData._commandPool, _contextData._allocator);

	// Pipeline
	vkDestroyPipelineCache(_deviceData._device, _windowData._pipelineCache, _contextData._allocator);
	//vkDestroyPipeline(_deviceData._device, _objects._pipeline, _contextData._allocator);
	//vkDestroyPipelineLayout(_deviceData._device, _objects._pipelineLayout, _contextData._allocator);
	vkDestroyRenderPass(_deviceData._device, _windowData._renderPass, _contextData._allocator);

	// Window
	vkDestroySwapchainKHR(_deviceData._device, _windowData._swapchain, _contextData._allocator);
	vkDestroySurfaceKHR(_contextData._instance, _windowData._surface, _contextData._allocator);
}

void VulkanRendererSystem::ResetVulkanWindow()
{
	DeleteVulkanWindow();
	CreateVulkanWindow(_windowData._windowData);
}

void VulkanRendererSystem::Clear()
{
	// TODO should there be something in clear function from renderer ?
}

VkCommandBuffer VulkanRendererSystem::PrepareDraw()
{
	vkWaitForFences(_deviceData._device, 1, &_windowData._frames[_windowData._currentFrame]._fence, VK_TRUE, UINT64_MAX);

	VkResult err = vkAcquireNextImageKHR(_deviceData._device, _windowData._swapchain, UINT64_MAX, _windowData._frames[_windowData._currentFrame]._presentComplete, VK_NULL_HANDLE, &_windowData._currentFrame);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || _windowData._windowData->_shouldUpdate)
	{
		ResetVulkanWindow();
		return VK_NULL_HANDLE;
	}
	check_vk_result(err);

	{
		VkCommandBufferBeginInfo commandBeginInfo = {};
		commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		commandBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		err = vkBeginCommandBuffer(_windowData._scene._commandBuffer, &commandBeginInfo);
		check_vk_result(err);

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = 512;
		viewport.height = 512;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = { 512, 512 };

		vkCmdSetViewport(_windowData._scene._commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(_windowData._scene._commandBuffer, 0, 1, &scissor);

		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

		VkRenderPassBeginInfo renderPassBeginInfo = {};
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = _windowData._scene._renderPass;
		renderPassBeginInfo.framebuffer = _windowData._scene._framebuffer;
		renderPassBeginInfo.renderArea.extent.width = 512;
		renderPassBeginInfo.renderArea.extent.height = 512;
		renderPassBeginInfo.clearValueCount = 1;
		renderPassBeginInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(_windowData._scene._commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		/*vkCmdBindPipeline(_windowData._scene._commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _objects._pipeline);

		ImGui::Begin("Demo");

		ImGui::InputFloat2("Vertex0 pos", (float*)&_objects.vertices[0].pos);
		ImGui::ColorEdit3("Vertex0 color", (float*)&_objects.vertices[0].color);

		ImGui::InputFloat2("Vertex1 pos", (float*)&_objects.vertices[1].pos);
		ImGui::ColorEdit3("Vertex1 color", (float*)&_objects.vertices[1].color);

		ImGui::InputFloat2("Vertex2 pos", (float*)&_objects.vertices[2].pos);
		ImGui::ColorEdit3("Vertex2 color", (float*)&_objects.vertices[2].color);

		ImGui::End();

		void* data;
		vkMapMemory(_deviceData._device, _objects._memory, 0, sizeof(_objects.vertices[0]) * _objects.vertices.size(), 0, &data);
		memcpy(data, _objects.vertices.data(), (size_t)sizeof(_objects.vertices[0]) * _objects.vertices.size());
		vkUnmapMemory(_deviceData._device, _objects._memory);

		VkBuffer vertexBuffers[] = { _objects._buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_windowData._scene._commandBuffer, 0, 1, vertexBuffers, offsets);

		vkCmdDraw(_windowData._scene._commandBuffer, _objects.vertices.size(), 1, 0, 0);*/

		vkCmdEndRenderPass(_windowData._scene._commandBuffer);
		VkResult err = vkEndCommandBuffer(_windowData._scene._commandBuffer);
		check_vk_result(err);
	}

	// EDITOR PASS
	VkCommandBufferBeginInfo commandBeginInfo = {};
	commandBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBeginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	err = vkBeginCommandBuffer(_windowData._frames[_windowData._currentFrame]._commandBuffer, &commandBeginInfo);
	check_vk_result(err);

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = _windowData._windowData->_width;
	viewport.height = _windowData._windowData->_height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = { _windowData._windowData->_width, _windowData._windowData->_height };

	vkCmdSetViewport(_windowData._frames[_windowData._currentFrame]._commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(_windowData._frames[_windowData._currentFrame]._commandBuffer, 0, 1, &scissor);

	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 0.0f };

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = _windowData._renderPass;
	renderPassBeginInfo.framebuffer = _windowData._frames[_windowData._currentFrame]._framebuffer;
	renderPassBeginInfo.renderArea.extent.width = _windowData._windowData->_width;
	renderPassBeginInfo.renderArea.extent.height = _windowData._windowData->_height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;
	vkCmdBeginRenderPass(_windowData._frames[_windowData._currentFrame]._commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	return _windowData._frames[_windowData._currentFrame]._commandBuffer;
}

void VulkanRendererSystem::SubmitDraw()
{
	// Submit command buffer
	vkCmdEndRenderPass(_windowData._frames[_windowData._currentFrame]._commandBuffer);

	VkResult err = vkEndCommandBuffer(_windowData._frames[_windowData._currentFrame]._commandBuffer);
	check_vk_result(err);
}

void VulkanRendererSystem::Render()
{
	TRACE("VulkanRendererSystem::Render")

		VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _windowData._frames[_windowData._currentFrame]._presentComplete };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	VkCommandBuffer buff[2];
	buff[0] = _windowData._frames[_windowData._currentFrame]._commandBuffer;
	buff[1] = _windowData._scene._commandBuffer;

	submitInfo.commandBufferCount = 2;
	submitInfo.pCommandBuffers = buff;

	VkSemaphore signalSemaphores[] = { _windowData._frames[_windowData._currentFrame]._renderComplete };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(_deviceData._device, 1, &_windowData._frames[_windowData._currentFrame]._fence);
	VkResult err = vkQueueSubmit(_deviceData._graphicsQueue, 1, &submitInfo, _windowData._frames[_windowData._currentFrame]._fence);
	check_vk_result(err);
}

void VulkanRendererSystem::Present()
{
	TRACE("VulkanRendererSystem::Present")

		VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &_windowData._frames[_windowData._currentFrame]._renderComplete;

	VkSwapchainKHR swapChains[] = { _windowData._swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &_windowData._currentFrame;

	presentInfo.pResults = nullptr; // Optional

	VkResult err = vkQueuePresentKHR(_deviceData._graphicsQueue, &presentInfo);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR || _windowData._windowData->_shouldUpdate)
		ResetVulkanWindow();
	else if (err != VK_SUCCESS)
		check_vk_result(err);

	_windowData._currentFrame = (_windowData._currentFrame + 1) % _windowData._frames.size();
}

VkCommandBuffer VulkanRendererSystem::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _windowData._commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_deviceData._device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanRendererSystem::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_deviceData._graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_deviceData._graphicsQueue);

	vkFreeCommandBuffers(_deviceData._device, _windowData._commandPool, 1, &commandBuffer);
}

void VulkanRendererSystem::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void VulkanRendererSystem::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void VulkanRendererSystem::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

void VulkanRendererSystem::CreateSceneRendering()
{
	// Color attachment
	VkImageCreateInfo image{};
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = VK_FORMAT_R8G8B8A8_SRGB;
	image.extent.width = 512;
	image.extent.height = 512;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	// We will sample directly from the color attachment
	image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

	check_vk_result(vkCreateImage(_deviceData._device, &image, nullptr, &_windowData._scene._image));

	VkMemoryAllocateInfo memAlloc{};
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(_deviceData._device, _windowData._scene._image, &memReqs);

	memAlloc.allocationSize = memReqs.size;

	uint32_t type = UINT32_MAX;
	for (uint32_t i = 0; i < _deviceData._memoryProperties.memoryTypeCount; i++) {
		if ((memReqs.memoryTypeBits & (1 << i)) && (_deviceData._memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			type = i;
		}
	}
	if (type == UINT32_MAX)
		throw;

	memAlloc.memoryTypeIndex = type;

	check_vk_result(vkAllocateMemory(_deviceData._device, &memAlloc, nullptr, &_windowData._scene._imageMemory));
	check_vk_result(vkBindImageMemory(_deviceData._device, _windowData._scene._image, _windowData._scene._imageMemory, 0));

	VkImageViewCreateInfo colorImageView{};
	colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	colorImageView.format = VK_FORMAT_R8G8B8A8_SRGB;
	colorImageView.subresourceRange = {};
	colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	colorImageView.subresourceRange.baseMipLevel = 0;
	colorImageView.subresourceRange.levelCount = 1;
	colorImageView.subresourceRange.baseArrayLayer = 0;
	colorImageView.subresourceRange.layerCount = 1;
	colorImageView.image = _windowData._scene._image;
	check_vk_result(vkCreateImageView(_deviceData._device, &colorImageView, nullptr, &_windowData._scene._imageView));

	// Create sampler to sample from the attachment in the fragment shader
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = samplerInfo.addressModeU;
	samplerInfo.addressModeW = samplerInfo.addressModeU;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	check_vk_result(vkCreateSampler(_deviceData._device, &samplerInfo, nullptr, &_windowData._scene._sampler));

	// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering

	std::array<VkAttachmentDescription, 1> attchmentDescriptions = {};
	// Color attachment
	attchmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_SRGB;
	attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Create the actual renderpass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
	renderPassInfo.pAttachments = attchmentDescriptions.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	check_vk_result(vkCreateRenderPass(_deviceData._device, &renderPassInfo, nullptr, &_windowData._scene._renderPass));

	VkImageView attachments[1];
	attachments[0] = _windowData._scene._imageView;

	VkFramebufferCreateInfo fbufCreateInfo{};
	fbufCreateInfo.renderPass = _windowData._scene._renderPass;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = attachments;
	fbufCreateInfo.width = 512;
	fbufCreateInfo.height = 512;
	fbufCreateInfo.layers = 1;

	check_vk_result(vkCreateFramebuffer(_deviceData._device, &fbufCreateInfo, nullptr, &_windowData._scene._framebuffer));

	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = _windowData._commandPool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = 1;
	check_vk_result(vkAllocateCommandBuffers(_deviceData._device, &info, &_windowData._scene._commandBuffer));

	_windowData._scene._set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(_windowData._scene._sampler, _windowData._scene._imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanRendererSystem::LoadTexture()
{

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load("D:/Personal project/EzUtils/Resources/Textures/chat.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(_deviceData._device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(_deviceData._device, stagingBuffer, &memRequirements);

		uint32_t type = UINT32_MAX;
		for (uint32_t i = 0; i < _deviceData._memoryProperties.memoryTypeCount; i++) {
			if ((memRequirements.memoryTypeBits & (1 << i)) && (_deviceData._memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
				type = i;
			}
		}
		if (type == UINT32_MAX)
			throw;

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = type;

		if (vkAllocateMemory(_deviceData._device, &allocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(_deviceData._device, stagingBuffer, stagingBufferMemory, 0);
	}

	void* data;
	vkMapMemory(_deviceData._device, stagingBufferMemory, 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(_deviceData._device, stagingBufferMemory);

	stbi_image_free(pixels);

	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(512);
	imageInfo.extent.height = static_cast<uint32_t>(512);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional

	if (vkCreateImage(_deviceData._device, &imageInfo, nullptr, &_windowData._texture._image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_deviceData._device, _windowData._texture._image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;

	uint32_t type = UINT32_MAX;
	for (uint32_t i = 0; i < _deviceData._memoryProperties.memoryTypeCount; i++) {
		if ((memRequirements.memoryTypeBits & (1 << i)) && (_deviceData._memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
			type = i;
		}
	}
	if (type == UINT32_MAX)
		throw;

	allocInfo.memoryTypeIndex = type;

	if (vkAllocateMemory(_deviceData._device, &allocInfo, nullptr, &_windowData._texture._imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(_deviceData._device, _windowData._texture._image, _windowData._texture._imageMemory, 0);

	transitionImageLayout(_windowData._texture._image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, _windowData._texture._image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	transitionImageLayout(_windowData._texture._image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = _windowData._texture._image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(_deviceData._device, &viewInfo, nullptr, &_windowData._texture._imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 16.0f;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

	samplerInfo.unnormalizedCoordinates = VK_FALSE;

	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(_deviceData._device, &samplerInfo, _contextData._allocator, &_windowData._texture._sampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	_windowData._texture._set = (VkDescriptorSet)ImGui_ImplVulkan_AddTexture(_windowData._texture._sampler, _windowData._texture._imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanRendererSystem::Debug()
{
	ImGui::Begin("Duno");
	ImGui::Image(_windowData._scene._set, ImGui::GetWindowSize());
	ImGui::End();

	ImGui::Begin("Vulkan Renderer System Info");

	if (ImGui::CollapsingHeader("Vulkan context"))
	{
		ImGui::Separator();
		ImGui::Columns(2);

		ImGui::Text("_instance"); ImGui::NextColumn();
		ImGui::Text("0x%p", _contextData._instance); ImGui::NextColumn();

		ImGui::Text("_allocator"); ImGui::NextColumn();
		ImGui::Text("0x%p", _contextData._allocator); ImGui::NextColumn();

		ImGui::Text("_debugMessenger"); ImGui::NextColumn();
		ImGui::Text("0x%p", _contextData._debugMessenger); ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Spacing();
	}

	if (ImGui::CollapsingHeader("Vulkan device"))
	{
		ImGui::Separator();
		ImGui::Columns(2);

		ImGui::Text("_physicalDevice"); ImGui::NextColumn();
		ImGui::Text("0x%p", _deviceData._physicalDevice); ImGui::NextColumn();

		ImGui::Text("_device"); ImGui::NextColumn();
		ImGui::Text("0x%p", _deviceData._device); ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Indent();
		if (ImGui::CollapsingHeader("_properties"))
		{
			ImGui::Columns(2);
			ImGui::Text("apiVersion"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._properties.apiVersion); ImGui::NextColumn();

			ImGui::Text("driverVersion"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._properties.driverVersion); ImGui::NextColumn();

			ImGui::Text("vendorID"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._properties.vendorID); ImGui::NextColumn();

			ImGui::Text("deviceID"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._properties.deviceID); ImGui::NextColumn();

			ImGui::Text("deviceType"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._properties.deviceType); ImGui::NextColumn();

			ImGui::Text("deviceName"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._properties.deviceName); ImGui::NextColumn();

			ImGui::Text("pipelineCacheUUID"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._properties.pipelineCacheUUID); ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Indent();
			if (ImGui::CollapsingHeader("limits"))
			{
				ImGui::Columns(2);
				ImGui::Text("maxImageDimension1D"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxImageDimension1D); ImGui::NextColumn();

				ImGui::Text("maxImageDimension2D"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxImageDimension2D); ImGui::NextColumn();

				ImGui::Text("maxImageDimension3D"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxImageDimension3D); ImGui::NextColumn();

				ImGui::Text("maxImageDimensionCube"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxImageDimensionCube); ImGui::NextColumn();

				ImGui::Text("maxImageArrayLayers"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxImageArrayLayers); ImGui::NextColumn();

				ImGui::Text("maxTexelBufferElements"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxTexelBufferElements); ImGui::NextColumn();

				ImGui::Text("maxUniformBufferRange"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxUniformBufferRange); ImGui::NextColumn();

				ImGui::Text("maxStorageBufferRange"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxStorageBufferRange); ImGui::NextColumn();

				ImGui::Text("maxPushConstantsSize"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxPushConstantsSize); ImGui::NextColumn();

				ImGui::Text("maxMemoryAllocationCount"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxMemoryAllocationCount); ImGui::NextColumn();

				ImGui::Text("maxSamplerAllocationCount"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxSamplerAllocationCount); ImGui::NextColumn();

				ImGui::Text("bufferImageGranularity"); ImGui::NextColumn();
				ImGui::Text("%llu", _deviceData._properties.limits.bufferImageGranularity); ImGui::NextColumn();

				ImGui::Text("maxTexelBufferElements"); ImGui::NextColumn();
				ImGui::Text("%llu", _deviceData._properties.limits.sparseAddressSpaceSize); ImGui::NextColumn();

				ImGui::Text("maxBoundDescriptorSets"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._properties.limits.maxBoundDescriptorSets); ImGui::NextColumn();
			}
			ImGui::Unindent();
			ImGui::Columns(2);
		}
		ImGui::Unindent();
		ImGui::Columns(2);

		ImGui::Columns(1);
		ImGui::Indent();
		if (ImGui::CollapsingHeader("_features"))
		{
			ImGui::Columns(2);
			ImGui::Text("robustBufferAccess"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._features.robustBufferAccess ? "true" : "false"); ImGui::NextColumn();

			ImGui::Text("fullDrawIndexUint32"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._features.fullDrawIndexUint32 ? "true" : "false"); ImGui::NextColumn();

			ImGui::Text("imageCubeArray"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._features.imageCubeArray ? "true" : "false"); ImGui::NextColumn();
		}
		ImGui::Unindent();
		ImGui::Columns(2);

		ImGui::Columns(1);
		ImGui::Indent();
		if (ImGui::CollapsingHeader("_enabledFeatures"))
		{
			ImGui::Columns(2);
			ImGui::Text("robustBufferAccess"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._enabledFeatures.robustBufferAccess ? "true" : "false"); ImGui::NextColumn();

			ImGui::Text("fullDrawIndexUint32"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._enabledFeatures.fullDrawIndexUint32 ? "true" : "false"); ImGui::NextColumn();

			ImGui::Text("imageCubeArray"); ImGui::NextColumn();
			ImGui::Text("%s", _deviceData._enabledFeatures.imageCubeArray ? "true" : "false"); ImGui::NextColumn();
		}
		ImGui::Unindent();
		ImGui::Columns(2);

		ImGui::Columns(1);
		ImGui::Indent();
		if (ImGui::CollapsingHeader("_memoryProperties"))
		{
			ImGui::Indent();
			for (int i = 0; i < _deviceData._memoryProperties.memoryTypeCount; ++i)
			{
				ImGui::Columns(1);

				if (ImGui::CollapsingHeader((std::string("memoryTypes ") + std::to_string(i)).c_str()))
				{
					ImGui::Columns(2);
					ImGui::Text("propertyFlags"); ImGui::NextColumn();
					ImGui::Text("%u", _deviceData._memoryProperties.memoryTypes[i].propertyFlags); ImGui::NextColumn();

					ImGui::Text("heapIndex"); ImGui::NextColumn();
					ImGui::Text("%u", _deviceData._memoryProperties.memoryTypes[i].heapIndex); ImGui::NextColumn();
				}
			}
			ImGui::Unindent();
			ImGui::Columns(2);

			ImGui::Indent();
			for (int i = 0; i < _deviceData._memoryProperties.memoryHeapCount; ++i)
			{
				ImGui::Columns(1);

				if (ImGui::CollapsingHeader((std::string("memoryHeaps ") + std::to_string(i)).c_str()))
				{
					ImGui::Columns(2);
					ImGui::Text("flags"); ImGui::NextColumn();
					ImGui::Text("%u", _deviceData._memoryProperties.memoryHeaps[i].flags); ImGui::NextColumn();

					ImGui::Text("memoryHeaps"); ImGui::NextColumn();
					ImGui::Text("%s", ez::GetReadableBytes(_deviceData._memoryProperties.memoryHeaps[i].size)); ImGui::NextColumn();
				}
			}
			ImGui::Unindent();
			ImGui::Columns(2);
		}
		ImGui::Unindent();
		ImGui::Columns(2);

		ImGui::Indent();
		for (int i = 0; i < _deviceData._queueFamilyProperties.size(); ++i)
		{
			ImGui::Columns(1);

			if (ImGui::CollapsingHeader((std::string("_queueFamilyProperties ") + std::to_string(i)).c_str()))
			{
				ImGui::Columns(2);
				ImGui::Text("queueFlags"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._queueFamilyProperties[i].queueFlags); ImGui::NextColumn();

				ImGui::Text("queueCount"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._queueFamilyProperties[i].queueCount); ImGui::NextColumn();

				ImGui::Text("timestampValidBits"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._queueFamilyProperties[i].timestampValidBits); ImGui::NextColumn();

				ImGui::Text("minImageTransferGranularity.width"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._queueFamilyProperties[i].minImageTransferGranularity.width); ImGui::NextColumn();

				ImGui::Text("minImageTransferGranularity.height"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._queueFamilyProperties[i].minImageTransferGranularity.height); ImGui::NextColumn();

				ImGui::Text("minImageTransferGranularity.depth"); ImGui::NextColumn();
				ImGui::Text("%u", _deviceData._queueFamilyProperties[i].minImageTransferGranularity.depth); ImGui::NextColumn();
			}
		}
		ImGui::Unindent();
		ImGui::Columns(2);

		ImGui::Columns(1);
		ImGui::Indent();
		if (ImGui::CollapsingHeader("_supportedExtensions"))
		{
			ImGui::Columns(2);
			for (int i = 0; i < _deviceData._supportedExtensions.size(); ++i)
			{
				ImGui::Text("_supportedExtensions %i", i); ImGui::NextColumn();
				ImGui::Text("%s", _deviceData._supportedExtensions[i].c_str()); ImGui::NextColumn();
			}
		}
		ImGui::Unindent();
		ImGui::Columns(2);

		ImGui::Columns(1);
		ImGui::Indent();
		if (ImGui::CollapsingHeader("_queueFamilyIndices"))
		{
			ImGui::Columns(2);
			ImGui::Text("_graphics"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._queueFamilyIndices._graphics); ImGui::NextColumn();

			ImGui::Text("_compute"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._queueFamilyIndices._compute); ImGui::NextColumn();

			ImGui::Text("_transfer"); ImGui::NextColumn();
			ImGui::Text("%u", _deviceData._queueFamilyIndices._transfer); ImGui::NextColumn();
		}
		ImGui::Unindent();
		ImGui::Columns(1);
		ImGui::Columns(2);

		ImGui::Text("_graphicsQueue"); ImGui::NextColumn();
		ImGui::Text("0x%p", _deviceData._graphicsQueue); ImGui::NextColumn();

		ImGui::Text("_computeQueue"); ImGui::NextColumn();
		ImGui::Text("0x%p", _deviceData._computeQueue); ImGui::NextColumn();

		ImGui::Text("_transferQueue"); ImGui::NextColumn();
		ImGui::Text("0x%p", _deviceData._transferQueue); ImGui::NextColumn();

		ImGui::Text("_descriptorPool"); ImGui::NextColumn();
		ImGui::Text("0x%p", _deviceData._descriptorPool); ImGui::NextColumn();

		ImGui::Columns(1);
		ImGui::Spacing();
	}

	if (ImGui::CollapsingHeader("Vulkan window"))
	{
		ImGui::Separator();
		ImGui::Columns(2);

		ImGui::Text("_surface"); ImGui::NextColumn();
		ImGui::Text("0x%p", _windowData._surface); ImGui::NextColumn();

		ImGui::Text("_colorFormat"); ImGui::NextColumn();
		ImGui::Text("%u", _windowData._colorFormat); ImGui::NextColumn();

		ImGui::Text("_colorSpace"); ImGui::NextColumn();
		ImGui::Text("%u", _windowData._colorSpace); ImGui::NextColumn();

		ImGui::Text("_swapchain"); ImGui::NextColumn();
		ImGui::Text("0x%p", _windowData._swapchain); ImGui::NextColumn();

		ImGui::Text("_presentMode"); ImGui::NextColumn();
		ImGui::Text("%u", _windowData._presentMode); ImGui::NextColumn();

		ImGui::Text("_imageCount"); ImGui::NextColumn();
		ImGui::Text("%u", _windowData._imageCount); ImGui::NextColumn();

		ImGui::Text("_currentFrame"); ImGui::NextColumn();
		ImGui::Text("%u", _windowData._currentFrame); ImGui::NextColumn();

		ImGui::Indent();
		for (int i = 0; i < _windowData._frames.size(); ++i)
		{
			ImGui::Columns(1);

			if (ImGui::CollapsingHeader((std::string("_frames ") + std::to_string(i)).c_str()))
			{
				ImGui::Columns(2);
				ImGui::Text("_commandBuffer"); ImGui::NextColumn();
				ImGui::Text("0x%p", _windowData._frames[i]._commandBuffer); ImGui::NextColumn();

				ImGui::Text("_image"); ImGui::NextColumn();
				ImGui::Text("0x%p", _windowData._frames[i]._image); ImGui::NextColumn();

				ImGui::Text("_imageView"); ImGui::NextColumn();
				ImGui::Text("0x%p", _windowData._frames[i]._imageView); ImGui::NextColumn();

				ImGui::Text("_framebuffer"); ImGui::NextColumn();
				ImGui::Text("0x%p", _windowData._frames[i]._framebuffer); ImGui::NextColumn();

				ImGui::Text("_fence"); ImGui::NextColumn();
				ImGui::Text("0x%p", _windowData._frames[i]._fence); ImGui::NextColumn();

				ImGui::Text("_presentComplete"); ImGui::NextColumn();
				ImGui::Text("0x%p", _windowData._frames[i]._presentComplete); ImGui::NextColumn();

				ImGui::Text("_renderComplete"); ImGui::NextColumn();
				ImGui::Text("0x%p", _windowData._frames[i]._renderComplete); ImGui::NextColumn();
			}
		}
		ImGui::Unindent();
		ImGui::Columns(1);
		ImGui::Columns(2);

		ImGui::Text("_commandPool"); ImGui::NextColumn();
		ImGui::Text("0x%p", _windowData._commandPool); ImGui::NextColumn();

		ImGui::Text("_renderPass"); ImGui::NextColumn();
		ImGui::Text("0x%p", _windowData._renderPass); ImGui::NextColumn();

		ImGui::Text("_pipelineCache"); ImGui::NextColumn();
		ImGui::Text("0x%p", _windowData._pipelineCache); ImGui::NextColumn();

		ImGui::Columns(1);
	}

	ImGui::End();
}