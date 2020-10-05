#include "Scene.h"

void Draw(const LogicalDevice& kLogicalDevice, const Scene& scene)
{
	// in the end, may use secondary buffer to avoid record scene foreach viewport
	for (int i = 0; i < scene._viewports.size(); ++i)
	{
		StartDraw(kLogicalDevice, *scene._viewports[i]);

		// foreach mesh draw
		for(int j = 0; j < scene._mesh.size(); ++j)
		{
			vkCmdBindPipeline(scene._viewports[i]->_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
								scene._mesh[j]->_material->_pipeline);

			void* data;
			vkMapMemory(kLogicalDevice._device, scene._mesh[j]->_memory, 0,
						sizeof(scene._mesh[j]->vertices[0]) * scene._mesh[j]->vertices.size(), 0, &data);
			memcpy(data, scene._mesh[j]->vertices.data(), (size_t)sizeof(scene._mesh[j]->vertices[0]) * scene._mesh[j]-> vertices.size());
			vkUnmapMemory(kLogicalDevice._device, scene._mesh[j]->_memory);

			VkBuffer vertexBuffers[] = { scene._mesh[j]->_buffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(scene._viewports[i]->_commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdDraw(scene._viewports[i]->_commandBuffer, scene._mesh[j]->vertices.size(), 1, 0, 0);
		}

		EndDraw(kLogicalDevice, *scene._viewports[i]);
	}
}