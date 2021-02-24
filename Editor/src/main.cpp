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
#include "Scene/Actor.h"

#include "Assets/AssetsMgr.h"

void LoadAssets()
{
	AssetsMgr<Texture>::load("skyboxCubemap", 
		std::array<std::string, 6>{ "D:/Personal project/DemoEngine/Resources/Textures/Cubemap/left.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/right.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/top.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/bottom.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/front.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/back.bmp" });

	AssetsMgr<Texture>::load("skyboxIradianceCubemap", 
		std::array<std::string, 6>{ "D:/Personal project/DemoEngine/Resources/Textures/Cubemap/left_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/right_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/top_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/bottom_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/front_irr.bmp",
			"D:/Personal project/DemoEngine/Resources/Textures/Cubemap/back_irr.bmp" });

	AssetsMgr<Texture>::load("brdf", "D:/Personal project/DemoEngine/Resources/Textures/brdf_lut.jpg");
	

	AssetsMgr<Texture>::load("color", "D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Color.jpg");
	AssetsMgr<Texture>::load("metal", "D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Metalness.jpg", Texture::Format::R);
	AssetsMgr<Texture>::load("normal", "D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Normal.jpg");
	AssetsMgr<Texture>::load("rough", "D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Roughness.jpg", Texture::Format::R);
	AssetsMgr<Texture>::load("aO", "D:/Personal project/DemoEngine/Resources/Textures/Metal007_2K_Displacement.jpg", Texture::Format::R);

	AssetsMgr<Mesh>::load("sphere", "D:/Personal project/DemoEngine/Resources/Mesh/sphere.obj");
	AssetsMgr<Mesh>::load("cube", "D:/Personal project/DemoEngine/Resources/Mesh/cube.obj");
	AssetsMgr<Mesh>::load("plane", "D:/Personal project/DemoEngine/Resources/Mesh/plane.obj");

	AssetsMgr<Mesh>::load("cubeSq", "D:/Personal project/DemoEngine/Resources/Mesh/cubeSq2.obj");
}

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
	cam._pos = { 0.f, 1.f, -2.f };
	Light light({ 0.f, 3.f, 1.f }, 1.f, { 1.f, 1.f, 1.f }, 5.f);

	AssetsMgr<Texture> txtMgr;
	AssetsMgr<Material> matMgr;
	AssetsMgr<Mesh> meshMgr;

	LoadAssets();

	AssetsMgr<Material>::load("skyboxMaterial", viewport,
		"D:/Personal project/DemoEngine/shaders/bin/skybox.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/skybox.frag.spv",
		std::vector<BindingsSet>{ { BindingsSet::Scope::GLOBAL, { { 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1 }, { 1, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 } }} },
		VK_CULL_MODE_FRONT_BIT, Vertex::POSITION);

	AssetsMgr<Material>::load("mat", viewport,
		"D:/Personal project/DemoEngine/shaders/bin/shader.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/shader.frag.spv",
		std::vector<BindingsSet>{ { BindingsSet::Scope::GLOBAL, { { 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1 }, { 1, Bindings::Stage::FRAGMENT, Bindings::Type::BUFFER, 1 },
			{ 2, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 }, { 3, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 },
			{ 4, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 } }},
		{ BindingsSet::Scope::MATERIAL, {{ 0, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 }, { 1, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 },
			{ 2, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 }, { 3, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 },
			{ 4, Bindings::Stage::FRAGMENT, Bindings::Type::SAMPLER, 1 }} },
		{ BindingsSet::Scope::ACTOR, {{ 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1 }} }
		});

	AssetsMgr<Material>::load("grid", viewport,
		"D:/Personal project/DemoEngine/shaders/bin/grid.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/grid.frag.spv",
		std::vector<BindingsSet>{ { BindingsSet::Scope::GLOBAL, { { 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1 } }} }, VK_CULL_MODE_BACK_BIT, Vertex::POSITION);
	
	AssetsMgr<Material>::load("gizmo", viewport,
		"D:/Personal project/DemoEngine/shaders/bin/gizmo.vert.spv",
		"D:/Personal project/DemoEngine/shaders/bin/gizmo.frag.spv",
		std::vector<BindingsSet>{ { BindingsSet::Scope::GLOBAL, { { 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1 } }}, 
		{ BindingsSet::Scope::ACTOR, {{ 0, Bindings::Stage::VERTEX, Bindings::Type::BUFFER, 1 }, { 1, Bindings::Stage::FRAGMENT, Bindings::Type::BUFFER, 1 }} } }, VK_CULL_MODE_BACK_BIT, Vertex::POSITION, true);

	MaterialInstance gridMat(AssetsMgr<Material>::get("grid"), { {&cam._ubo} });
	Actor grid(AssetsMgr<Mesh>::get("plane"), gridMat);

	MaterialInstance skyMaterialInstance(AssetsMgr<Material>::get("skyboxMaterial"), { { &cam._ubo, &AssetsMgr<Texture>::get("skyboxCubemap") } });
	Actor skySphere(AssetsMgr<Mesh>::get("sphere"), skyMaterialInstance);

	MaterialInstance matInstance1(AssetsMgr<Material>::get("mat"), 
		{ { &cam._ubo, &light._ubo, &AssetsMgr<Texture>::get("skyboxCubemap"), &AssetsMgr<Texture>::get("skyboxIradianceCubemap"), &AssetsMgr<Texture>::get("brdf") },
		{ &AssetsMgr<Texture>::get("color"), &AssetsMgr<Texture>::get("metal"), &AssetsMgr<Texture>::get("normal"), &AssetsMgr<Texture>::get("rough"),
		&AssetsMgr<Texture>::get("aO")} });

	MaterialInstance matInstance2(AssetsMgr<Material>::get("mat"),
		{ { &cam._ubo, &light._ubo, &AssetsMgr<Texture>::get("skyboxCubemap"), &AssetsMgr<Texture>::get("skyboxIradianceCubemap"), &AssetsMgr<Texture>::get("brdf") },
		{ &AssetsMgr<Texture>::get("color"), &AssetsMgr<Texture>::get("metal"), &AssetsMgr<Texture>::get("normal"), &AssetsMgr<Texture>::get("rough"),
		&AssetsMgr<Texture>::get("aO")} });

	Actor mesh(AssetsMgr<Mesh>::get("sphere"), matInstance1);
	matInstance1.UpdateSet(2, { { &mesh._transform._buffer } });

	Actor second(AssetsMgr<Mesh>::get("cube"), matInstance2);
	matInstance2.UpdateSet(2, { { &second._transform._buffer } });

	MaterialInstance gizmoMat(AssetsMgr<Material>::get("gizmo"), { { &cam._ubo } });

	Buffer colorBuffer(sizeof(Vec3), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	Vec3 col{ 1.0f, 0.f, 0.f };
	colorBuffer.Map(&col, sizeof(Vec3));

	Actor gizmo(AssetsMgr<Mesh>::get("cubeSq"), gizmoMat);
	gizmoMat.UpdateSet(1, { { &gizmo._transform._buffer }, { &colorBuffer } });

	second._transform.Translate({ 0.f, 0.f, 2.5f });
	gizmo._transform.Translate({ 0.f, 3.f, 1.f });

	Scene scene{};
	scene._viewports.emplace_back(&viewport);
	scene._actors.emplace_back(&mesh);
	scene._actors.emplace_back(&second);
	scene._actors.emplace_back(&gizmo);
	scene._actors.emplace_back(&skySphere);
	scene._actors.emplace_back(&grid);

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
			cam._pos += cam._rot * Vec3{0.f, 0.f, -1.f} * deltaTime;
		else if(windowData->IsKeyDown(KEY_CODE::W))
			cam._pos += cam._rot * Vec3{ 0.f, 0.f, 1.f } * deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::A))
			cam._pos += cam._rot * Vec3{ -1.f, 0.f, 0.f } * deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::D))
			cam._pos += cam._rot * Vec3{ 1.f, 0.f, 0.f } * deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::Q))
			cam._pos += cam._rot * Vec3{ 0.f, -1.f, 0.f } * deltaTime;
		else if (windowData->IsKeyDown(KEY_CODE::E))
			cam._pos += cam._rot * Vec3{ 0.f, 1.f, 0.f } * deltaTime;

		if (windowData->IsMouseDown(MOUSE_CODE::RIGHT))
		{
			Vec2 deltaPos = windowData->_mousePos - mousePos;
			if (fabs(deltaPos.x) > fabs(deltaPos.y))
				deltaPos.y = 0;
			else
				deltaPos.x = 0;
			cam._rot *= Quat(Vec3{ deltaPos.y, deltaPos.x, 0 } * deltaTime);
		}
		mousePos = windowData->_mousePos;
		
		cam.Update();

		mesh._transform.Rotate(Quat(deltaTime * glm::radians(10.0f) * Vec3{ 0.f, 1.f, 0.f }));

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