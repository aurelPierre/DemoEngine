#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

#include "Swapchain.h"

int main(int, char**)
{
	GLFWWindowSystem		glfwWindow;
	ImGuiSystem				imGui;
	GLFWWindowData*			windowData	= glfwWindow.CreateWindow();

	Context context = CreateContext();
	Device device = CreateDevice(context);
	LogicalDevice logicalDevice = CreateLogicalDevice(context, device);
	Surface surface = CreateSurface(context, logicalDevice, device, windowData);
	Swapchain swapchain = CreateSwapchain(context, logicalDevice, device, surface, windowData);

	imGui.Init(windowData, context, device, logicalDevice, swapchain);

	Viewport viewport = CreateViewport(context, logicalDevice, device, surface._colorFormat, { 512, 512 });
	AddViewport(viewport, swapchain);

	while (!glfwWindow.UpdateInput() ) // TODO create window abstraction
	{
		// Clear
		imGui.StartFrame();

		if (windowData->_shouldUpdate)
		{
			ResizeSwapchain(context, logicalDevice, device, surface, windowData, swapchain);
			windowData->_shouldUpdate = false;
		}

		// Update


		// Draw
		ez::LogSystem::Draw();
		ez::ProfileSystem::Draw();

		if(!AcquireNextImage(logicalDevice, swapchain))
		{
			windowData->_shouldUpdate = true;
			imGui.EndFrame();
			continue;
		}

		for (int i = 0; i < swapchain._viewports.size(); ++i)
		{
			if (!Draw(logicalDevice, swapchain._viewports[i]))
				ResizeViewport(context, logicalDevice, device, surface._colorFormat, swapchain._viewports[i]);
			else
				Render(logicalDevice, swapchain._viewports[i]);
		}

		Draw(logicalDevice, swapchain);
		Render(logicalDevice, swapchain);
		if(!Present(logicalDevice, swapchain))
			windowData->_shouldUpdate = true;

		imGui.EndFrame();
	}

	imGui.Clear();

	DestroySwapchain(context, logicalDevice, swapchain);
	DestroySurface(context, surface);
	DestroyLogicalDevice(context, logicalDevice);
	DestroyContext(context);

	glfwWindow.DeleteWindow();

	return EXIT_SUCCESS;
}