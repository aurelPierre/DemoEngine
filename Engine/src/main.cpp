#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

#include "VkRenderer/Swapchain.h"
#include "Scene/Mesh.h"
#include "VkRenderer/Texture.h"
#include "Scene/Scene.h"

#include "VkRenderer/Core.h"

#include "VkRenderer/Context.h"

int main(int, char**)
{
	GLFWWindowSystem		glfwWindow;
	ImGuiSystem				imGui;
	GLFWWindowData*			windowData	= glfwWindow.CreateWindow();

	Context context;
	Device device;
	LogicalDevice logicalDevice(device);
	Surface surface(windowData);
	Swapchain swapchain(surface, windowData);

	imGui.Init(windowData, context, device, logicalDevice, swapchain);

	Viewport viewport(surface._colorFormat, { 512, 512 });

	Texture color("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Color.jpg");
	Texture metal("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Metalness.jpg");
	Texture normal("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Normal.jpg");
	Texture rough("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Roughness.jpg");
	Texture aO("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Displacement.jpg");

	Material mat(viewport,
		"D:/Personal project/DemoEngine/shaders/bin/shader.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/shader.frag.spv",
		{ &color, &metal, &normal, &rough, &aO });

	Mesh mesh("D:/Personal project/DemoEngine/Resources/Mesh/sphere.obj");
	mesh._material = &mat;

	Scene scene{};
	scene._viewports.emplace_back(&viewport);
	scene._mesh.emplace_back(&mesh);
	
	static bool deviceWindow = true;
	imGui._globalFunctions.emplace_back([&device]() {
		DrawWindow("Device data", device, &deviceWindow);
	});

	bool lDeviceWindow = true;
	imGui._globalFunctions.emplace_back([&logicalDevice, &lDeviceWindow]() {
		DrawWindow("LogicalDevice data", logicalDevice, &lDeviceWindow);
	});

	bool viewportWindow = true;
	imGui._globalFunctions.emplace_back([&viewport, &viewportWindow]() {
		DrawWindow("viewport DEMO", viewport, &viewportWindow);
	});

	while (!glfwWindow.UpdateInput() ) // TODO create window abstraction
	{
		TRACE("main::loop")

		// Clear
		imGui.StartFrame();

		if (windowData->_shouldUpdate)
		{
			swapchain.Resize(surface, windowData);
			for (size_t i = 0; i < scene._viewports.size(); ++i)
			{
				if (!scene._viewports[i]->UpdateViewportSize())
					scene._viewports[i]->Resize(surface._colorFormat);
			}
			windowData->_shouldUpdate = false;
		}

		// Update
		

		// Draw
		Draw(scene);

		ez::LogSystem::Draw();
		ez::ProfileSystem::Draw();

		if(!swapchain.AcquireNextImage())
		{
			windowData->_shouldUpdate = true;
			imGui.EndFrame();
			continue;
		}

		for (size_t i = 0; i < scene._viewports.size(); ++i)
		{
			if (!scene._viewports[i]->UpdateViewportSize())
				scene._viewports[i]->Resize(surface._colorFormat);
			else
				scene._viewports[i]->Render();
		}
		
		swapchain.Draw();
		swapchain.Render();
		if(!swapchain.Present())
			windowData->_shouldUpdate = true;

		imGui.EndFrame();
	}

	vkDeviceWaitIdle(logicalDevice._device);

	imGui.Clear();

	glfwWindow.DeleteWindow();

	ez::LogSystem::Save();

	return EXIT_SUCCESS;
}