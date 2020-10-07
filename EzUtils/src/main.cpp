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

	Context context = CreateContext();
	Device device = CreateDevice(context);
	LogicalDevice logicalDevice = CreateLogicalDevice(context, device);
	Surface surface = CreateSurface(context, logicalDevice, device, windowData);
	Swapchain swapchain = CreateSwapchain(context, logicalDevice, device, surface, windowData);

	imGui.Init(windowData, context, device, logicalDevice, swapchain);

	Viewport viewport = CreateViewport(context, logicalDevice, device, surface._colorFormat, { 512, 512 });

	Texture mugColor = CreateTexture(context, logicalDevice, device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_BaseColor.png");
	Texture mugMetal = CreateTexture(context, logicalDevice, device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Metallic.png");
	Texture mugNormal = CreateTexture(context, logicalDevice, device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Normal.png");
	Texture mugRough = CreateTexture(context, logicalDevice, device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Roughness.png");
	Texture mugAO = CreateTexture(context, logicalDevice, device,
		"D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Default_AmbientOcclusion.png");

	Material mat = CreateMaterial(context, logicalDevice, device, viewport,
		"D:/Personal project/DemoEngine/shaders/bin/shader.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/shader.frag.spv",
		{ &mugColor, &mugMetal, &mugNormal, &mugRough, &mugAO });

	Mesh mesh = CreateMesh(context, logicalDevice, device, "D:/Personal project/DemoEngine/Resources/Mesh/HylianShield.obj");
	mesh._material = &mat;


	Scene scene{};
	scene._viewports.emplace_back(&viewport);
	scene._mesh.emplace_back(&mesh);

	while (!glfwWindow.UpdateInput() ) // TODO create window abstraction
	{
		TRACE("main::loop")

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

	DestroyTexture(context, logicalDevice, mugColor);
	DestroyTexture(context, logicalDevice, mugMetal);
	DestroyTexture(context, logicalDevice, mugNormal);
	DestroyTexture(context, logicalDevice, mugRough);
	DestroyTexture(context, logicalDevice, mugAO);

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