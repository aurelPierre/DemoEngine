#pragma once

#include <vulkan/vulkan.h>

// Might want to move debug out of this structure
// It doesn't make sense that it is tied to the context
// A structure handling every vulkan debug settings should be better

// Might also want to move allocator out of thi structure
// A structure with custom memory management could be usefull
struct Context
{
	VkInstance					_instance		= VK_NULL_HANDLE;
	VkAllocationCallbacks*		_allocator		= nullptr;
	VkDebugUtilsMessengerEXT	_debugMessenger = VK_NULL_HANDLE; 
};

Context CreateContext();
void	DestroyContext(const Context& kContext);
