#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

#include "Swapchain.h"
#include "Mesh.h"
#include "Texture.h"
#include "Scene.h"

int main(int, char**)
{
	GLFWWindowSystem		glfwWindow;
	ImGuiSystem				imGui;
	GLFWWindowData*			windowData	= glfwWindow.CreateWindow();

	Context context;
	Device device;
	LogicalDevice logicalDevice(device);
	Surface surface(device, windowData);
	Swapchain swapchain(device, surface, windowData);

	imGui.Init(windowData, context, device, logicalDevice, swapchain);

	Viewport viewport(device, surface._colorFormat, { 512, 512 });

	Texture mugColor(device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_BaseColor.png");
	Texture mugMetal(device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Metallic.png");
	Texture mugNormal(device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Normal.png");
	Texture mugRough(device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Roughness.png");
	Texture mugAO(device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Default_AmbientOcclusion.png");

	Material mat(device, viewport,
		"D:/Personal project/DemoEngine/shaders/bin/shader.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/shader.frag.spv",
		{ &mugColor, &mugMetal, &mugNormal, &mugRough, &mugAO });

	Mesh mesh(device, "D:/Personal project/DemoEngine/Resources/Mesh/HylianShield.obj");
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
			swapchain.Resize(device, surface, windowData);
			for (int i = 0; i < scene._viewports.size(); ++i)
			{
				if (!scene._viewports[i]->UpdateViewportSize())
					scene._viewports[i]->Resize(device, surface._colorFormat);
			}
			windowData->_shouldUpdate = false;
		}

		// Update
		

		// Draw
		Draw(logicalDevice, scene);

		ez::LogSystem::Draw();
		ez::ProfileSystem::Draw();

		if(!swapchain.AcquireNextImage())
		{
			windowData->_shouldUpdate = true;
			imGui.EndFrame();
			continue;
		}

		for (int i = 0; i < scene._viewports.size(); ++i)
		{
			if (!scene._viewports[i]->UpdateViewportSize())
				scene._viewports[i]->Resize(device, surface._colorFormat);
			else
				scene._viewports[i]->Render();
		}
		
		swapchain.Draw();
		swapchain.Render();
		if(!swapchain.Present())
			windowData->_shouldUpdate = true;

		imGui.EndFrame();
	}

	imGui.Clear();


	glfwWindow.DeleteWindow();

	return EXIT_SUCCESS;
}