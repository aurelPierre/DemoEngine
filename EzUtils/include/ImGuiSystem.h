#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <string>
#include <future>
#include <vector>

struct GLFWWindowData;

class Context;
class Device;
class LogicalDevice;
class Swapchain;

template<typename T>
struct WindowRef
{
	T*		_data;
	bool	_open;
};

class ImGuiSystem
{
public:
	std::vector<std::packaged_task<void()>> _menuFunctions;

	std::vector<std::packaged_task<void()>> _globalFunctions;

	void Init(const GLFWWindowData*, const Context&, const Device&,
			const LogicalDevice&, const Swapchain&);

	void StartFrame();
	void Draw(VkCommandBuffer command);
	void EndFrame();
	void Clear();
};

