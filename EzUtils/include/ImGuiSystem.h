#pragma once

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

struct GLFWWindowData;

struct VulkanContext;
struct VulkanDevice;
struct VulkanWindow;

struct ImGuiData
{

};

class ImGuiSystem
{
public:

	void Init(const GLFWWindowData const *, const VulkanContext const *, const VulkanDevice const *, const VulkanWindow const *);

	void StartFrame();
	void Draw(VkCommandBuffer command);
	void EndFrame();
	void Clear();
};