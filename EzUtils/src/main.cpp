#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

#include "Swapchain.h"
#include "Mesh.h"
#include "Scene.h"

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

	Material mat = CreateMaterial(context, logicalDevice, viewport,
		"D:/Personal project/EzUtils/shaders/bin/shader.vert.spv", "D:/Personal project/EzUtils/shaders/bin/shader.frag.spv");

	Mesh mesh = CreateMesh(context, logicalDevice, device);
	mesh._material = &mat;

	Scene scene{};
	scene._viewports.emplace_back(&viewport);
	scene._mesh.emplace_back(&mesh);

	while (!glfwWindow.UpdateInput() ) // TODO create window abstraction
	{
		// Clear
		imGui.StartFrame();

		if (windowData->_shouldUpdate)
		{
			ResizeSwapchain(context, logicalDevice, device, surface, windowData, swapchain);
			for (int i = 0; i < scene._viewports.size(); ++i)
			{
				if (!UpdateViewportSize(*scene._viewports[i]))
					ResizeViewport(context, logicalDevice, device, surface._colorFormat, *scene._viewports[i]);
			}
			windowData->_shouldUpdate = false;
		}

		// Update
		

		// Draw
		Draw(logicalDevice, scene);

		ez::LogSystem::Draw();
		ez::ProfileSystem::Draw();

		if(!AcquireNextImage(logicalDevice, swapchain))
		{
			windowData->_shouldUpdate = true;
			imGui.EndFrame();
			continue;
		}

		for (int i = 0; i < scene._viewports.size(); ++i)
		{
			if (!UpdateViewportSize(*scene._viewports[i]))
				ResizeViewport(context, logicalDevice, device, surface._colorFormat, *scene._viewports[i]);
			else
				Render(logicalDevice, *scene._viewports[i]);
		}
		
		Draw(logicalDevice, swapchain);
		Render(logicalDevice, swapchain);
		if(!Present(logicalDevice, swapchain))
			windowData->_shouldUpdate = true;

		imGui.EndFrame();
	}

	imGui.Clear();

	DestroyMesh(context, logicalDevice, mesh);
	DestroyMaterial(context, logicalDevice, mat);

	DestroyViewport(context, logicalDevice, viewport);

	DestroySwapchain(context, logicalDevice, swapchain);
	DestroySurface(context, surface);
	DestroyLogicalDevice(context, logicalDevice);
	DestroyContext(context);

	glfwWindow.DeleteWindow();

	return EXIT_SUCCESS;
}