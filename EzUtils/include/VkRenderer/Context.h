#pragma once

#include <vulkan/vulkan.h>

#include "Editor.h"

// Might want to move debug out of this structure
// It doesn't make sense that it is tied to the context
// A structure handling every vulkan debug settings should be better

// Might also want to move allocator out of thi structure
// A structure with custom memory management could be usefull
class Context
{
	static const Context* _sInstance;

public:
	VkInstance					_instance		= VK_NULL_HANDLE;
	VkAllocationCallbacks*		_allocator		= nullptr;
	VkDebugUtilsMessengerEXT	_debugMessenger = VK_NULL_HANDLE;

public:
	static const Context& Instance();

public:
	Context();
	~Context();
};

template<>
inline void DrawEditor<Context>(const Context& obj)
{
	ImGui::Columns(2);

	ImGui::Text("_instance"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._instance); ImGui::NextColumn();

	ImGui::Text("_allocator"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._allocator); ImGui::NextColumn();

	ImGui::Text("_debugMessenger"); ImGui::NextColumn();
	ImGui::Text("0x%p", obj._debugMessenger); ImGui::NextColumn();

	ImGui::Columns(1);
}
