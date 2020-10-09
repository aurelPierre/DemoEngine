#pragma once

#include <vulkan/vulkan.h>

#include "Context.h"
#include "Editor.h"

#include <vector>

struct Queue
{
	uint32_t		_indice			= 0;
	VkQueue			_queue			= VK_NULL_HANDLE;
	VkCommandPool	_commandPool	= VK_NULL_HANDLE;
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

	VkDescriptorPool	_descriptorPool		= VK_NULL_HANDLE;
};

uint32_t	RateDeviceSuitability(const VkPhysicalDevice& kDevice);

Device				CreateDevice(const Context& kContext);
LogicalDevice		CreateLogicalDevice(const Context& kContext, const Device& kDevice);

void DestroyLogicalDevice(const Context& kContext, const LogicalDevice& kLogicalDevice);

template<>
inline void DrawEditor<VkPhysicalDeviceProperties>(const VkPhysicalDeviceProperties& obj)
{
	ImGui::Columns(2);
	ImGui::Text("apiVersion"); ImGui::NextColumn();
	ImGui::Text("%u", obj.apiVersion); ImGui::NextColumn();

	ImGui::Text("driverVersion"); ImGui::NextColumn();
	ImGui::Text("%u", obj.driverVersion); ImGui::NextColumn();

	ImGui::Text("vendorID"); ImGui::NextColumn();
	ImGui::Text("%u", obj.vendorID); ImGui::NextColumn();

	ImGui::Text("deviceID"); ImGui::NextColumn();
	ImGui::Text("%u", obj.deviceID); ImGui::NextColumn();

	ImGui::Text("deviceType"); ImGui::NextColumn();
	ImGui::Text("%u", obj.deviceType); ImGui::NextColumn();

	ImGui::Text("deviceName"); ImGui::NextColumn();
	ImGui::Text("%s", obj.deviceName); ImGui::NextColumn();

	ImGui::Text("pipelineCacheUUID"); ImGui::NextColumn();
	ImGui::Text("%s", obj.pipelineCacheUUID); ImGui::NextColumn();

	ImGui::Columns(1);
	if (ImGui::CollapsingHeader("limits"))
	{
		ImGui::Indent();
		ImGui::Columns(2);
		ImGui::Text("maxImageDimension1D"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxImageDimension1D); ImGui::NextColumn();

		ImGui::Text("maxImageDimension2D"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxImageDimension2D); ImGui::NextColumn();

		ImGui::Text("maxImageDimension3D"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxImageDimension3D); ImGui::NextColumn();

		ImGui::Text("maxImageDimensionCube"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxImageDimensionCube); ImGui::NextColumn();

		ImGui::Text("maxImageArrayLayers"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxImageArrayLayers); ImGui::NextColumn();

		ImGui::Text("maxTexelBufferElements"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxTexelBufferElements); ImGui::NextColumn();

		ImGui::Text("maxUniformBufferRange"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxUniformBufferRange); ImGui::NextColumn();

		ImGui::Text("maxStorageBufferRange"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxStorageBufferRange); ImGui::NextColumn();

		ImGui::Text("maxPushConstantsSize"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxPushConstantsSize); ImGui::NextColumn();

		ImGui::Text("maxMemoryAllocationCount"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxMemoryAllocationCount); ImGui::NextColumn();

		ImGui::Text("maxSamplerAllocationCount"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxSamplerAllocationCount); ImGui::NextColumn();

		ImGui::Text("bufferImageGranularity"); ImGui::NextColumn();
		ImGui::Text("%llu", obj.limits.bufferImageGranularity); ImGui::NextColumn();

		ImGui::Text("maxTexelBufferElements"); ImGui::NextColumn();
		ImGui::Text("%llu", obj.limits.sparseAddressSpaceSize); ImGui::NextColumn();

		ImGui::Text("maxBoundDescriptorSets"); ImGui::NextColumn();
		ImGui::Text("%u", obj.limits.maxBoundDescriptorSets); ImGui::NextColumn();

		ImGui::Unindent();
	}
	ImGui::Columns(1);
}

template<>
inline void DrawEditor<VkPhysicalDeviceFeatures>(const VkPhysicalDeviceFeatures& obj)
{
	ImGui::Columns(2);
	ImGui::Text("robustBufferAccess"); ImGui::NextColumn();
	ImGui::Text("%s", obj.robustBufferAccess ? "true" : "false"); ImGui::NextColumn();

	ImGui::Text("fullDrawIndexUint32"); ImGui::NextColumn();
	ImGui::Text("%s", obj.fullDrawIndexUint32 ? "true" : "false"); ImGui::NextColumn();

	ImGui::Text("imageCubeArray"); ImGui::NextColumn();
	ImGui::Text("%s", obj.imageCubeArray ? "true" : "false"); ImGui::NextColumn();
	ImGui::Columns(1);
}

template<>
inline void DrawEditor<VkPhysicalDeviceMemoryProperties>(const VkPhysicalDeviceMemoryProperties& obj)
{
	for (int i = 0; i < obj.memoryTypeCount; ++i)
	{
		if (ImGui::CollapsingHeader((std::string("memoryTypes ") + std::to_string(i)).c_str()))
		{
			ImGui::Indent();
			ImGui::Columns(2);

			ImGui::Text("propertyFlags"); ImGui::NextColumn();
			ImGui::Text("%u", obj.memoryTypes[i].propertyFlags); ImGui::NextColumn();

			ImGui::Text("heapIndex"); ImGui::NextColumn();
			ImGui::Text("%u", obj.memoryTypes[i].heapIndex); ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Unindent();
		}
	}

	for (int i = 0; i < obj.memoryHeapCount; ++i)
	{
		if (ImGui::CollapsingHeader((std::string("memoryHeaps ") + std::to_string(i)).c_str()))
		{
			ImGui::Indent();
			ImGui::Columns(2);

			ImGui::Text("flags"); ImGui::NextColumn();
			ImGui::Text("%u", obj.memoryHeaps[i].flags); ImGui::NextColumn();

			ImGui::Text("memoryHeaps"); ImGui::NextColumn();
			ImGui::Text("%s", ez::GetReadableBytes(obj.memoryHeaps[i].size)); ImGui::NextColumn();

			ImGui::Columns(1);
			ImGui::Unindent();
		}
	}
}

template<>
inline void DrawEditor<VkQueueFamilyProperties>(const VkQueueFamilyProperties& obj)
{
	ImGui::Columns(2);
	ImGui::Text("queueFlags"); ImGui::NextColumn();
	ImGui::Text("%u", obj.queueFlags); ImGui::NextColumn();

	ImGui::Text("queueCount"); ImGui::NextColumn();
	ImGui::Text("%u", obj.queueCount); ImGui::NextColumn();

	ImGui::Text("timestampValidBits"); ImGui::NextColumn();
	ImGui::Text("%u", obj.timestampValidBits); ImGui::NextColumn();

	ImGui::Text("minImageTransferGranularity.width"); ImGui::NextColumn();
	ImGui::Text("%u", obj.minImageTransferGranularity.width); ImGui::NextColumn();

	ImGui::Text("minImageTransferGranularity.height"); ImGui::NextColumn();
	ImGui::Text("%u", obj.minImageTransferGranularity.height); ImGui::NextColumn();

	ImGui::Text("minImageTransferGranularity.depth"); ImGui::NextColumn();
	ImGui::Text("%u", obj.minImageTransferGranularity.depth); ImGui::NextColumn();
	ImGui::Columns(1);
}

template<>
inline void DrawEditor<Device>(const Device& obj)
{
	ImGui::Columns(2);
	ImGui::Text("_physicalDevice"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._physicalDevice); ImGui::NextColumn();
	ImGui::Columns(1);

	if (ImGui::CollapsingHeader("_properties"))
	{
		ImGui::Indent();
		DrawEditor(obj._properties);
		ImGui::Unindent();
	}
	if (ImGui::CollapsingHeader("_features"))
	{
		ImGui::Indent();
		DrawEditor(obj._features);
		ImGui::Unindent();
	}
	if (ImGui::CollapsingHeader("_memoryProperties"))
	{
		ImGui::Indent();
		DrawEditor(obj._memoryProperties);
		ImGui::Unindent();
	}
	for (int i = 0; i < obj._queueFamilyProperties.size(); ++i)
	{
		if (ImGui::CollapsingHeader((std::string("_queueFamilyProperties ") + std::to_string(i)).c_str()))
		{
			ImGui::Indent();
			DrawEditor(obj._queueFamilyProperties[i]);
			ImGui::Unindent();
		}
	}
	if (ImGui::CollapsingHeader("_supportedExtensions"))
	{
		ImGui::Indent();
		for (int i = 0; i < obj._supportedExtensions.size(); ++i)
			ImGui::Text("%s", obj._supportedExtensions[i].c_str());
		ImGui::Unindent();
	}
}

template<>
inline void DrawEditor<Queue>(const Queue& obj)
{
	ImGui::Columns(2);

	ImGui::Text("_indice"); ImGui::NextColumn();
	ImGui::Text("%u", obj._indice); ImGui::NextColumn();

	ImGui::Text("_queue"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._queue); ImGui::NextColumn();

	ImGui::Text("_commandPool"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._commandPool); ImGui::NextColumn();

	ImGui::Columns(1);
}

template<>
inline void DrawEditor<LogicalDevice>(const LogicalDevice& obj)
{
	ImGui::Columns(2);

	ImGui::Text("_device"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._device); ImGui::NextColumn();

	ImGui::Text("_descriptorPool"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._descriptorPool); ImGui::NextColumn();

	ImGui::Columns(1);
	
	if (ImGui::CollapsingHeader("_graphicsQueue"))
	{
		ImGui::Indent();
		DrawEditor(obj._graphicsQueue);
		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("_transferQueue"))
	{
		ImGui::Indent();
		DrawEditor(obj._transferQueue);
		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("_computeQueue"))
	{
		ImGui::Indent();
		DrawEditor(obj._computeQueue);
		ImGui::Unindent();
	}
};