#include "Scene.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include "ImGuiSystem.h"

void Draw(const LogicalDevice& kLogicalDevice, const Scene& scene)
{
	UniformBufferObject ubo{};
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	ImGui::Begin("Scene");

	ImGui::BeginGroup();
	ImGui::Text("LookAt");
	static float eye[3]{ 15.f, 6.f, 15.f };
	ImGui::InputFloat3("eye", eye);
	static float center[3]{ 0.f, 1.5f, 0.f };
	ImGui::InputFloat3("center", center);
	static float up[3]{ 0.f, 1.f, 0.f };
	ImGui::InputFloat3("up", up);

	ubo.view = glm::lookAt(glm::vec3(eye[0], eye[1], eye[2]), glm::vec3(center[0], center[1], center[2]), 
							glm::vec3(up[0], up[1], up[2]));
	
	ImGui::EndGroup();

	static float rUp[3]{ 0.f, 1.f, 0.f };


	// in the end, may use secondary buffer to avoid record scene foreach viewport
	for (int i = 0; i < scene._viewports.size(); ++i)
	{
		ubo.proj = glm::perspective(glm::radians(60.0f), (float)(scene._viewports[i]->_size.width / (float)scene._viewports[i]->_size.height),
			0.1f, 512.0f);
		ubo.proj[1][1] *= -1;

		scene._viewports[i]->StartDraw();

		// foreach mesh draw
		for (int j = 0; j < scene._mesh.size(); ++j)
		{
			ImGui::BeginGroup();
			ImGui::Text("Model");
			ImGui::InputFloat3("rUp", rUp);
			ImGui::EndGroup();

			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(rUp[0], rUp[1], rUp[2]));

			scene._mesh[j]->_material->_ubo.Map(&ubo, sizeof(UniformBufferObject));
			scene._mesh[j]->Draw(scene._viewports[i]->_commandBuffer._commandBuffer);
		}

		scene._viewports[i]->EndDraw();
	}

	ImGui::End();

}