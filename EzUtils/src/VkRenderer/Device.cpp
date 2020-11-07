#include "Device.h"

#include "Core.h"

#include <map>

uint32_t Device::RateDeviceSuitability(const VkPhysicalDevice& kDevice)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceProperties(kDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(kDevice, &deviceFeatures);

	uint32_t score = 0;

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

Device::Device()
{
	uint32_t gpu_count;
	VkResult err = vkEnumeratePhysicalDevices(Context::Instance()._instance, &gpu_count, NULL);
	check_vk_result(err);
	//IM_ASSERT(gpu_count > 0);

	std::vector<VkPhysicalDevice> physicalDevices(gpu_count);
	err = vkEnumeratePhysicalDevices(Context::Instance()._instance, &gpu_count, physicalDevices.data());
	check_vk_result(err);

	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : physicalDevices) {
		int score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0) {
		_physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(_physicalDevice, &_properties);
	vkGetPhysicalDeviceFeatures(_physicalDevice, &_features);
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &_memoryProperties);

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
	//assert(queueFamilyCount > 0);
	_queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, _queueFamilyProperties.data());

	// Get list of supported extensions
	uint32_t extCount = 0;
	err = vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extCount, nullptr);
	check_vk_result(err);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);

		err = vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extCount, &extensions.front());
		check_vk_result(err);

		for (VkExtensionProperties& ext : extensions)
			_supportedExtensions.push_back(ext.extensionName);
	}
}

const LogicalDevice* LogicalDevice::_sInstance = nullptr;

const LogicalDevice& LogicalDevice::Instance()
{
	return *_sInstance;
}

LogicalDevice::LogicalDevice(const Device& kDevice)
{
	_sInstance = this;

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Get queue family indices for the requested queue family types
	// Note that the indices may overlap depending on the implementation

	const float defaultQueuePriority{ 0.f };

	for (uint32_t i = 0; i < static_cast<uint32_t>(kDevice._queueFamilyProperties.size()); i++)
	{
		if (kDevice._queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			_graphicsQueue._indice = i;
		else if (kDevice._queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			_computeQueue._indice= i;
		else if (kDevice._queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			_transferQueue._indice = i;
	}

	// Graphics queue
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = _graphicsQueue._indice;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	// Dedicated compute queue
	{
		if (_graphicsQueue._indice != _computeQueue._indice)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = _computeQueue._indice;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}

	// Dedicated transfer queue
	{
		if ((_transferQueue._indice != _graphicsQueue._indice)
			&& (_transferQueue._indice != _computeQueue._indice))
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = _transferQueue._indice;
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
	deviceCreateInfo.pEnabledFeatures = &kDevice._features;

	deviceCreateInfo.enabledExtensionCount = (uint32_t)deviceExtensions.size();
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// to ensure older implementation compatibility
	deviceCreateInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();

	VkResult err = vkCreateDevice(kDevice._physicalDevice, &deviceCreateInfo, Context::Instance()._allocator, &_device);
	check_vk_result(err);

	vkGetDeviceQueue(_device, _graphicsQueue._indice, 0, &_graphicsQueue._queue);
	vkGetDeviceQueue(_device, _computeQueue._indice, 0, &_computeQueue._queue);
	vkGetDeviceQueue(_device, _transferQueue._indice, 0, &_transferQueue._queue);

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = _graphicsQueue._indice;
	err = vkCreateCommandPool(_device, &info, Context::Instance()._allocator,
								&_graphicsQueue._commandPool);
	check_vk_result(err);

	if (_computeQueue._indice != _graphicsQueue._indice)
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = _computeQueue._indice;
		err = vkCreateCommandPool(_device, &info, Context::Instance()._allocator, &_computeQueue._commandPool);
		check_vk_result(err);
	}
	else
		_computeQueue._commandPool = _graphicsQueue._commandPool;
	
	if (_transferQueue._indice == _graphicsQueue._indice)
	{
		_transferQueue._commandPool = _graphicsQueue._commandPool;
	}
	else if (_transferQueue._indice == _computeQueue._indice)
	{
		_transferQueue._commandPool = _computeQueue._commandPool;
	}
	else
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = _transferQueue._indice;
		err = vkCreateCommandPool(_device, &info, Context::Instance()._allocator, &_transferQueue._commandPool);
		check_vk_result(err);
	}

	{
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
		err = vkCreateDescriptorPool(_device, &pool_info, Context::Instance()._allocator, &_descriptorPool);
		check_vk_result(err);
	}
}

LogicalDevice::~LogicalDevice()
{
	vkDestroyDescriptorPool(_device, _descriptorPool, Context::Instance()._allocator);
	vkDestroyCommandPool(_device, _graphicsQueue._commandPool, Context::Instance()._allocator);

	if (_computeQueue._indice != _graphicsQueue._indice)
		vkDestroyCommandPool(_device, _computeQueue._commandPool, Context::Instance()._allocator);

	if (_transferQueue._indice != _graphicsQueue._indice && _transferQueue._indice != _computeQueue._indice)
		vkDestroyCommandPool(_device, _transferQueue._commandPool, Context::Instance()._allocator);

	vkDestroyDevice(_device, Context::Instance()._allocator);
}