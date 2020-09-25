#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

struct GLFWWindowData;

struct Context;
struct Device;
struct LogicalDevice;
struct Swapchain;

struct ImGuiData
{

};

class ImGuiSystem
{
public:

	void Init(const GLFWWindowData const *, const Context&, const Device&,
			const LogicalDevice&, const Swapchain&);

	void StartFrame();
	void Draw(VkCommandBuffer command);
	void EndFrame();
	void Clear();
};