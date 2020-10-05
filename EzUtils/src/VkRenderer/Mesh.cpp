#include "Mesh.h"


Mesh	CreateMesh(const Context& kContext, const LogicalDevice& kLogicalDevice, const Device& kDevice)
{
	Mesh mesh{};

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(mesh.vertices[0]) * mesh.vertices.size();
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(kLogicalDevice._device, &bufferInfo, kContext._allocator, &mesh._buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(kLogicalDevice._device, mesh._buffer, &memRequirements);

	uint32_t type = UINT32_MAX;
	for (uint32_t i = 0; i < kDevice._memoryProperties.memoryTypeCount; i++) {
		if ((memRequirements.memoryTypeBits & (1 << i)) && (kDevice._memoryProperties.memoryTypes[i].propertyFlags & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) == (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
			type = i;
		}
	}
	if (type == UINT32_MAX)
		throw;

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = type;

	if (vkAllocateMemory(kLogicalDevice._device, &allocInfo, kContext._allocator, &mesh._memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(kLogicalDevice._device, mesh._buffer, mesh._memory, 0);

	return mesh;
}

void	Draw(const LogicalDevice& kLogicalDevice, const Mesh& kMesh)
{
	
}

void	DestroyMesh(const Context& kContext, const LogicalDevice& kLogicalDevice, const Mesh& kMesh)
{
	vkFreeMemory(kLogicalDevice._device, kMesh._memory, kContext._allocator);
	vkDestroyBuffer(kLogicalDevice._device, kMesh._buffer, kContext._allocator);
}