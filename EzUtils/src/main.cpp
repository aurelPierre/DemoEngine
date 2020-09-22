#include "VulkanRendererSystem.h"
#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

int main(int, char**)
{
	/****************/
	/***** INIT *****/
	/****************/
	GLFWWindowSystem		glfwWindow;
	VulkanRendererSystem	vulkanRenderer;
	ImGuiSystem				imGui;

	GLFWWindowData*			windowData		= glfwWindow.CreateWindow();
	VulkanContext*			contextData		= vulkanRenderer.CreateVulkanContext();
	VulkanDevice*			deviceData		= vulkanRenderer.CreateVulkanDevice();
	VulkanWindow*			vkWindowData	= vulkanRenderer.CreateVulkanWindow(windowData);

	imGui.Init(windowData, contextData, deviceData, vkWindowData);

	vulkanRenderer.CreateSceneRendering();

	/****************/
	/***** LOOP *****/
	/****************/
	while (!glfwWindow.UpdateInput() ) // TODO create window abstraction
	{
		// Clear
		imGui.StartFrame();
		vulkanRenderer.Clear();
		
		// Update

		// Draw
		VkCommandBuffer command = vulkanRenderer.PrepareDraw();
		if (command == VK_NULL_HANDLE) // Resize
		{
			imGui.Draw(command);
			imGui.EndFrame();
			continue;
		}

		ez::LogSystem::Draw();
		ez::ProfileSystem::Draw();
		vulkanRenderer.Debug();

		imGui.Draw(command);

		vulkanRenderer.SubmitDraw();

		// Render
		vulkanRenderer.Render();

		// Present
		vulkanRenderer.Present();
		
		imGui.EndFrame();
	}

	/*****************/
	/***** CLEAR *****/
	/*****************/
	vulkanRenderer.DeleteVulkanWindow();

	imGui.Clear();
	
	vulkanRenderer.DeleteVulkanDevice();
	vulkanRenderer.DeleteVulkanContext();

	glfwWindow.DeleteWindow();

	return EXIT_SUCCESS;
}