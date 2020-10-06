#include "Scene.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

void Draw(const LogicalDevice& kLogicalDevice, const Scene& scene)
{
	// in the end, may use secondary buffer to avoid record scene foreach viewport
	for (int i = 0; i < scene._viewports.size(); ++i)
	{
		StartDraw(kLogicalDevice, *scene._viewports[i]);

		// foreach mesh draw
		for (int j = 0; j < scene._mesh.size(); ++j)
		{
			static auto startTime = std::chrono::high_resolution_clock::now();

			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

			UniformBufferObject ubo{};
			ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.view = glm::lookAt(glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(60.0f), (float)(scene._viewports[i]->_size.width / (float)scene._viewports[i]->_size.height),
				0.1f, 512.0f);
			ubo.proj[1][1] *= -1;

			void* data;
			vkMapMemory(kLogicalDevice._device, scene._mesh[j]->_material->_uboMemory, 0,
				sizeof(UniformBufferObject), 0, &data);
			memcpy(data, &ubo, sizeof(UniformBufferObject));
			vkUnmapMemory(kLogicalDevice._device, scene._mesh[j]->_material->_uboMemory);

			Draw(kLogicalDevice, *scene._mesh[j], scene._viewports[i]->_commandBuffer);
		}

		EndDraw(kLogicalDevice, *scene._viewports[i]);
	}
}