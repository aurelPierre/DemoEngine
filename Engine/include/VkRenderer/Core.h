#pragma once

#include "LogSystem.h"

#include <string>

#include <vulkan/vulkan.h>
#include <fstream>

#ifndef NDEBUG
	#define ASSERT(predicate, msg) \
		if(!(predicate)) \
		{ \
			LOG(ez::ASSERT, std::string(__FILE__) + std::to_string(__LINE__) + std::string(msg)) \
			ez::LogSystem::Save(); \
			std::abort(); \
		}
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
