#include "GLFWWindowSystem.h"
#include "ImGuiSystem.h"
#include "LogSystem.h"
#include "Utils.h"

#include "VkRenderer/Swapchain.h"
#include "VkRenderer/Texture.h"

#include "Core.h"

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

	Camera cam(60.f, 0.1f, 256.f);
	cam._pos = { 0.f, 2.f, 0.f };
	Light light({ 0.f, 3.f, 1.f }, 1.f, { 1.f, 1.f, 1.f }, 5.f);

	Buffer modelBuf(sizeof(Mat4), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	Mat4 model(1.f);
	modelBuf.Map(&model, sizeof(Mat4));

	Texture skyCubemap({ "D:/Personal project/DemoEngine/Resources/Textures/Cubemap/left.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/right.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/top.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/bottom.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/front.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/back.bmp" });

	Texture skyIrradianceCubemap({ "D:/Personal project/DemoEngine/Resources/Textures/Cubemap/left_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/right_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/top_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/bottom_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/front_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/back_irr.bmp" });

	Texture brdf("D:/Personal project/DemoEngine/Resources/Textures/brdf_lut.jpg");

	Material skyMaterial(viewport,
		"D:/Personal project/DemoEngine/shaders/bin/skybox.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/skybox.frag.spv",
		{ { { 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1, &cam._ubo }, 
		{ 1, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &skyCubemap } } },
		VK_CULL_MODE_FRONT_BIT);

	Mesh skySphere("D:/Personal project/DemoEngine/Resources/Mesh/cube.obj");
	skySphere._material = &skyMaterial;

	Texture color("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Color.jpg");
	Texture metal("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Metalness.jpg", Texture::Format::R);
	Texture normal("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Normal.jpg");
	Texture rough("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Roughness.jpg", Texture::Format::R);
	Texture aO("D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Displacement.jpg", Texture::Format::R);

	Material mat(viewport,
		"D:/Personal project/DemoEngine/shaders/bin/shader.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/shader.frag.spv",
		{ { { 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1, &cam._ubo }, { 1, Bindings::Stage::FRAGMENT, Bindings::Type::BUFFER, 1, &light._ubo }, 
			{ 2, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &skyCubemap }, { 3, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &skyIrradianceCubemap }, 
			{ 4, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &brdf }},
		{ { 0, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &color }, { 1, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &metal },
			{ 2, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &normal }, { 3, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &rough },
			{ 4, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1, &aO }},
		{ { 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1, &modelBuf }, }
		});

	Mesh mesh("D:/Personal project/DemoEngine/Resources/Mesh/sphere.obj");
	mesh._material = &mat;

	Scene scene{};
	scene._viewports.emplace_back(&viewport);
	scene._mesh.emplace_back(&mesh);
	scene._mesh.emplace_back(&skySphere);

	/*static bool camWindow = true;
	imGui._globalFunctions.emplace_back([&viewport, &cam]() {
		DrawWindow("Scene", viewport, &cam);
	});

	static bool lightWindow = true;
	imGui._globalFunctions.emplace_back([&viewport, &light]() {
		DrawWindow("Scene", viewport, &light);
	});*/

	Vec2 mousePos;

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

		if (windowData->IsKeyDown(KEY_CODE::S))
			cam._pos += cam._rot * Vec3{0.f, -1.f, 0.f} * deltaTime;
		else if(windowData->IsKeyDown(KEY_CODE::W))
			cam._pos += cam._rot * Vec3{ 0.f, 1.f, 0.f } * deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::A))
			cam._pos += cam._rot * Vec3{ -1.f, 0.f, 0.f } *deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::D))
			cam._pos += cam._rot * Vec3{ 1.f, 0.f, 0.f } *deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::Q))
			cam._pos += cam._rot * Vec3{ 0.f, 0.f, -1.f } *deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::E))
			cam._pos += cam._rot * Vec3{ 0.f, 0.f, 1.f } *deltaTime;

		if (windowData->IsMouseDown(MOUSE_CODE::RIGHT))
		{
			Vec2 deltaPos = windowData->_mousePos - mousePos;
			cam._rot *= Quat(Vec3{ deltaPos.y, 0, deltaPos.x } * deltaTime * -1.f);
		}
		mousePos = windowData->_mousePos;
		
		cam.Update();

		model = glm::rotate(model, deltaTime * glm::radians(20.0f), { 0.f, 0.f, 1.f });
		modelBuf.Map(&model, sizeof(Mat4));

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