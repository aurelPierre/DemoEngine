#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

#include "VkRenderer/Swapchain.h"
#include "VkRenderer/Texture.h"

#include "VkRenderer/Core.h"

#include "VkRenderer/Context.h"

#include "Scene/Camera.h"
#include "Scene/Light.h"
#include "Scene/Mesh.h"
#include "Scene/Scene.h"

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

	Texture color("D:/Personal project/DemoEngine/Resources/Textures/HylianShield_BaseColor.png");
	Texture metal("D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Metallic.png");
	Texture normal("D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Normal.png");
	Texture rough("D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Roughness.png");
	Texture aO("D:/Personal project/DemoEngine/Resources/Textures/HylianShield_Default_AmbientOcclusion.png");

	Material mat(viewport,
		"D:/Personal project/DemoEngine/shaders/bin/shader.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/shader.frag.spv",
		5);
	
	Mesh mesh("D:/Personal project/DemoEngine/Resources/Mesh/sphere.obj");
	mesh._material = &mat;

	Scene scene{};
	scene._viewports.emplace_back(&viewport);
	scene._mesh.emplace_back(&mesh);

	Camera cam(60.f, 0.1f, 256.f);
	cam._pos = { 0.f, 2.f, 0.f };
	cam.Update();
	Light light({ 0.f, 3.f, 1.f }, 1.f, { 1.f, 1.f, 1.f }, 5.f);

	Buffer modelBuf(sizeof(glm::mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	glm::mat4 model(1.f);
	modelBuf.Map(&model, sizeof(glm::mat4));

	mat.UpdateDescriptors( cam._ubo, light._ubo, { &color, &metal, &normal, &rough, &aO }, modelBuf);

	/*static bool camWindow = true;
	imGui._globalFunctions.emplace_back([&viewport, &cam]() {
		DrawWindow("Scene", viewport, &cam);
	});

	static bool lightWindow = true;
	imGui._globalFunctions.emplace_back([&viewport, &light]() {
		DrawWindow("Scene", viewport, &light);
	});*/

	ez::Timer time;
	float deltaTime = 0.f;
	while (!glfwWindow.UpdateInput() ) // TODO create window abstraction
	{
		TRACE("main::loop")
		
		deltaTime = time.Duration<std::chrono::seconds::period>();
		time.Start();

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
		
		ImGui::Begin("Demo");
		float p[]{ cam._pos.x, cam._pos.y, cam._pos.z };
		ImGui::InputFloat3("Camera pos:", p);
		cam._pos = { p[0], p[1], p[2] };
		ImGui::End();
		cam.Update();

		model = glm::rotate(model, deltaTime * glm::radians(20.0f), { 0.f, 0.f, 1.f });
		modelBuf.Map(&model, sizeof(glm::mat4));

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
		time.Stop();
	}

	vkDeviceWaitIdle(logicalDevice._device);

	imGui.Clear();

	glfwWindow.DeleteWindow();

	ez::LogSystem::Save();

	return EXIT_SUCCESS;
}