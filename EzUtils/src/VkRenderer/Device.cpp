#include "Device.h"

#include "Core.h"

#include <map>

uint32_t	RateDeviceSuitability(const VkPhysicalDevice& kDevice)
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

Device	CreateDevice(const Context& kContext)
{
	Device newDevice{};

	uint32_t gpu_count;
	VkResult err = vkEnumeratePhysicalDevices(kContext._instance, &gpu_count, NULL);
	check_vk_result(err);
	//IM_ASSERT(gpu_count > 0);

	std::vector<VkPhysicalDevice> physicalDevices(gpu_count);
	err = vkEnumeratePhysicalDevices(kContext._instance, &gpu_count, physicalDevices.data());
	check_vk_result(err);

	// Use an ordered map to automatically sort candidates by increasing score
	std::multimap<int, VkPhysicalDevice> candidates;

	for (const auto& device : physicalDevices) {
		int score = RateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	// Check if the best candidate is suitable at all
	if (candidates.rbegin()->first > 0) {
		newDevice._physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(newDevice._physicalDevice, &newDevice._properties);
	vkGetPhysicalDeviceFeatures(newDevice._physicalDevice, &newDevice._features);
	vkGetPhysicalDeviceMemoryProperties(newDevice._physicalDevice, &newDevice._memoryProperties);

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(newDevice._physicalDevice, &queueFamilyCount, nullptr);
	//assert(queueFamilyCount > 0);
	newDevice._queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(newDevice._physicalDevice, &queueFamilyCount, newDevice._queueFamilyProperties.data());

	// Get list of supported extensions
	uint32_t extCount = 0;
	err = vkEnumerateDeviceExtensionProperties(newDevice._physicalDevice, nullptr, &extCount, nullptr);
	check_vk_result(err);
	if (extCount > 0)
	{
		std::vector<VkExtensionProperties> extensions(extCount);

		err = vkEnumerateDeviceExtensionProperties(newDevice._physicalDevice, nullptr, &extCount, &extensions.front());
		check_vk_result(err);

		for (VkExtensionProperties& ext : extensions)
			newDevice._supportedExtensions.push_back(ext.extensionName);
	}

	return newDevice;
}

LogicalDevice	CreateLogicalDevice(const Context& kContext, const Device& kDevice)
{
	LogicalDevice newLogicalDevice{};

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

	// Get queue family indices for the requested queue family types
	// Note that the indices may overlap depending on the implementation

	const float defaultQueuePriority{ 0.f };

	for (uint32_t i = 0; i < static_cast<uint32_t>(kDevice._queueFamilyProperties.size()); i++)
	{
		if (kDevice._queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			newLogicalDevice._graphicsQueue._indice = i;
		else if (kDevice._queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			newLogicalDevice._computeQueue._indice= i;
		else if (kDevice._queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			newLogicalDevice._transferQueue._indice = i;
	}

	// Graphics queue
	{
		VkDeviceQueueCreateInfo queueInfo{};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = newLogicalDevice._graphicsQueue._indice;
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = &defaultQueuePriority;
		queueCreateInfos.push_back(queueInfo);
	}

	// Dedicated compute queue
	{
		if (newLogicalDevice._graphicsQueue._indice != newLogicalDevice._computeQueue._indice)
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = newLogicalDevice._computeQueue._indice;
			queueInfo.queueCount = 1;
			queueInfo.pQueuePriorities = &defaultQueuePriority;
			queueCreateInfos.push_back(queueInfo);
		}
	}

	// Dedicated transfer queue
	{
		if ((newLogicalDevice._transferQueue._indice != newLogicalDevice._graphicsQueue._indice)
			&& (newLogicalDevice._transferQueue._indice != newLogicalDevice._computeQueue._indice))
		{
			// If compute family index differs, we need an additional queue create info for the compute queue
			VkDeviceQueueCreateInfo queueInfo{};
			queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex = newLogicalDevice._transferQueue._indice;
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

	VkResult err = vkCreateDevice(kDevice._physicalDevice, &deviceCreateInfo, kContext._allocator, &newLogicalDevice._device);
	check_vk_result(err);

	vkGetDeviceQueue(newLogicalDevice._device, newLogicalDevice._graphicsQueue._indice, 0, &newLogicalDevice._graphicsQueue._queue);
	vkGetDeviceQueue(newLogicalDevice._device, newLogicalDevice._computeQueue._indice, 0, &newLogicalDevice._computeQueue._queue);
	vkGetDeviceQueue(newLogicalDevice._device, newLogicalDevice._transferQueue._indice, 0, &newLogicalDevice._transferQueue._queue);

	VkCommandPoolCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	info.queueFamilyIndex = newLogicalDevice._graphicsQueue._indice;
	err = vkCreateCommandPool(newLogicalDevice._device, &info, kContext._allocator, 
								&newLogicalDevice._graphicsQueue._commandPool);
	check_vk_result(err);

	if (newLogicalDevice._computeQueue._indice != newLogicalDevice._graphicsQueue._indice)
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = newLogicalDevice._computeQueue._indice;
		err = vkCreateCommandPool(newLogicalDevice._device, &info, kContext._allocator, &newLogicalDevice._computeQueue._commandPool);
		check_vk_result(err);
	}
	else
		newLogicalDevice._computeQueue._commandPool = newLogicalDevice._graphicsQueue._commandPool;
	
	if (newLogicalDevice._transferQueue._indice == newLogicalDevice._graphicsQueue._indice)
	{
		newLogicalDevice._transferQueue._commandPool = newLogicalDevice._graphicsQueue._commandPool;
	}
	else if (newLogicalDevice._transferQueue._indice == newLogicalDevice._computeQueue._indice)
	{
		newLogicalDevice._transferQueue._commandPool = newLogicalDevice._computeQueue._commandPool;
	}
	else
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		info.queueFamilyIndex = newLogicalDevice._transferQueue._indice;
		err = vkCreateCommandPool(newLogicalDevice._device, &info, kContext._allocator, &newLogicalDevice._transferQueue._commandPool);
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
		err = vkCreateDescriptorPool(newLogicalDevice._device, &pool_info, kContext._allocator, &newLogicalDevice._descriptorPool);
		check_vk_result(err);
	}

	return newLogicalDevice;
}

void DestroyLogicalDevice(const Context& kContext, const LogicalDevice& kLogicalDevice)
{
	vkDestroyDescriptorPool(kLogicalDevice._device, kLogicalDevice._descriptorPool, kContext._allocator);
	vkDestroyCommandPool(kLogicalDevice._device, kLogicalDevice._graphicsQueue._commandPool, kContext._allocator);

	if (kLogicalDevice._computeQueue._indice != kLogicalDevice._graphicsQueue._indice)
		vkDestroyCommandPool(kLogicalDevice._device, kLogicalDevice._computeQueue._commandPool, kContext._allocator);

	if (kLogicalDevice._transferQueue._indice != kLogicalDevice._graphicsQueue._indice && kLogicalDevice._transferQueue._indice != kLogicalDevice._computeQueue._indice)
		vkDestroyCommandPool(kLogicalDevice._device, kLogicalDevice._transferQueue._commandPool, kContext._allocator);

	vkDestroyDevice(kLogicalDevice._device, kContext._allocator);
}