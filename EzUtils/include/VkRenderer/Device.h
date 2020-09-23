#pragma once

#include <vulkan/vulkan.h>

#include "Context.h"

#include <vector>

struct Queue
{
	uint32_t	_indice		= 0;
	VkQueue		_queue		= VK_NULL_HANDLE;
};

struct Device
{
	VkPhysicalDevice						_physicalDevice			= VK_NULL_HANDLE;
	VkPhysicalDeviceProperties				_properties;
	VkPhysicalDeviceFeatures				_features;
	VkPhysicalDeviceMemoryProperties		_memoryProperties;
	std::vector<VkQueueFamilyProperties>	_queueFamilyProperties;
	std::vector<std::string>				_supportedExtensions;
};

struct LogicalDevice
{
	VkDevice			_device				= VK_NULL_HANDLE;
	Queue				_graphicsQueue;
	Queue				_computeQueue;
	Queue				_transferQueue;
};

uint32_t	RateDeviceSuitability(const VkPhysicalDevice& kDevice);

Device				CreateDevice(const Context& kContext);
LogicalDevice		CreateLogicalDevice(const Context& kContext, const Device& kDevice);

void DestroyLogicalDevice(const Context& kContext, const LogicalDevice& kLogicalDevice);