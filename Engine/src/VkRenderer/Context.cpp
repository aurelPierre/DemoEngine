#include "VkRenderer/Context.h"

#include "VkRenderer/Core.h"

#include "GLFWWindowSystem.h"

const Context* Context::_sInstance = nullptr;

const Context& Context::Instance()
{
	ASSERT(_sInstance != nullptr, "_sInstance is nullptr")
	return *_sInstance;
}

Context::Context()
{
	_sInstance = this;

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Demo";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "DemoEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t layerCount;
	VkResult err = vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	VK_ASSERT(err, "error when enumerating instance layer properties");

	std::vector<VkLayerProperties> availableLayers(layerCount);
	err = vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	VK_ASSERT(err, "error when enumerating instance layer properties");

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		ASSERT(layerFound, "validation layers " + std::string(layerName) + " requested, but not available!")
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

	err = vkCreateInstance(&createInfo, _allocator, &_instance);
	VK_ASSERT(err, "error when creating instance");

	/************ Debug ************/
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional

		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
		ASSERT(func != nullptr, "vkCreateDebugUtilsMessengerEXT requested, but not available!")

		VkResult err = func(_instance, &createInfo, _allocator, &_debugMessenger);
		VK_ASSERT(err, "error when getting instance proc addr");
	}
}

Context::~Context()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
	ASSERT(func != nullptr, "vkDestroyDebugUtilsMessengerEXT requested, but not available!")
	func(_instance, _debugMessenger, _allocator);

	vkDestroyInstance(_instance, _allocator);
}