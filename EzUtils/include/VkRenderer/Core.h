#pragma once

#include "LogSystem.h"

#include <string>

#include <vulkan/vulkan.h>
#include <fstream>

#ifndef NDEBUG
	#define ASSERT(predicate, msg) if(!(predicate)) LOG(ez::ASSERT, std::string(__FILE__) + std::to_string(__LINE__) + std::string(msg)) else abort;
#else
	#define ASSERT(predicate, msg)	
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

void check_vk_result(const VkResult err);

std::vector<char> readFile(const std::string& filename);

uint32_t findMemoryType(VkPhysicalDeviceMemoryProperties memProperties, uint32_t typeFilter, VkMemoryPropertyFlags properties);

VkFormat findDepthFormat(const VkPhysicalDevice& kPhysicalDevice);
VkFormat findSupportedFormat(const VkPhysicalDevice& kPhysicalDevice, const std::vector<VkFormat>& candidates,
							VkImageTiling tiling, VkFormatFeatureFlags features);