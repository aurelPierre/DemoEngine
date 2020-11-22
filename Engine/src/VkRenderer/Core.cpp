#include "VkRenderer/Core.h"

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	switch (messageSeverity)
	{
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		LOG(ez::INFO, std::string("validation layer: ") + pCallbackData->pMessage)
			break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		LOG(ez::INFO, std::string("validation layer: ") + pCallbackData->pMessage)
			break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		LOG(ez::WARNING, std::string("validation layer: ") + pCallbackData->pMessage)
			break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		LOG(ez::ERROR, std::string("validation layer: ") + pCallbackData->pMessage)
			break;
	default:
		LOG(ez::CRITICAL, std::string("validation layer: ") + pCallbackData->pMessage)
			break;
	}

	return VK_FALSE;
}

std::vector<char> readFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	ASSERT(file.is_open(), "failed to open file " + filename + " !")

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

