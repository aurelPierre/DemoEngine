#include "Core.h"

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

void check_vk_result(const VkResult& err)
{
	if (err == VK_SUCCESS)
		return;

	throw std::runtime_error("VkResult " + std::to_string((int)err));
}