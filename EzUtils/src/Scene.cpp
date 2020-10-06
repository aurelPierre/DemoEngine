#include "Scene.h"

#define GLM_FORCE_RADIANS
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
			ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			ubo.proj = glm::perspective(glm::radians(60.0f), (float)(scene._viewports[i]->_size.width / (float)scene._viewports[i]->_size.height),
				0.1f, 512.0f);
			ubo.proj[1][1] *= -1;

			void* data;
			vkMapMemory(kLogicalDevice._device, scene._mesh[j]->_material->_uboMemory, 0,
				sizeof(UniformBufferObject), 0, &data);
			memcpy(data, &ubo, sizeof(UniformBufferObject));
			vkUnmapMemory(kLogicalDevice._device, scene._mesh[j]->_material->_uboMemory);

			vkCmdBindPipeline(scene._viewports[i]->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								scene._mesh[j]->_material->_pipeline);
			vkCmdBindDescriptorSets(scene._viewports[i]->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				scene._mesh[j]->_material->_pipelineLayout, 0, 1, &scene._mesh[j]->_material->_uboSet, 0, nullptr);
			
			vkCmdBindIndexBuffer(scene._viewports[i]->_commandBuffer, scene._mesh[j]->_indicesBuffer, 0, VK_INDEX_TYPE_UINT32);
			VkDeviceSize offset[]{ 0 };
			vkCmdBindVertexBuffers(scene._viewports[i]->_commandBuffer, 0, 1, &scene._mesh[j]->_verticesBuffer, offset);

			vkCmdDrawIndexed(scene._viewports[i]->_commandBuffer, scene._mesh[j]->_indices.size(), 1, 0, 0, 0);

			//Draw(kLogicalDevice, *scene._mesh[j], scene._viewports[i]->_commandBuffer);
		}

		EndDraw(kLogicalDevice, *scene._viewports[i]);
	}
}