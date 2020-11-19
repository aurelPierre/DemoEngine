#pragma once

#include "LogSystem.h"

#include <string>

#include <vulkan/vulkan.h>
#include <fstream>

#ifndef NDEBUG
	#define ASSERT(predicate, msg) \
		if(!(predicate)) \
		{ \
			LOG(ez::ASSERT, std::string(__FILE__) + ':' + std::to_string(__LINE__) + "\n\t" + std::string(msg) + '\n') \
			ez::LogSystem::Save(); \
			std::abort(); \
		}

	#define VK_ASSERT(err, msg) ASSERT(err == VK_SUCCESS, msg)
#else
	#define ASSERT(predicate, msg)
	
	#define VK_ASSERT(err, msg) (void)err;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

std::vector<char> readFile(const std::string& filename);
