#include "Buffer.h"

#include "Context.h"
#include "Core.h"

Buffer::Buffer(const Device& kDevice, const VkDeviceSize kSize, const VkBufferUsageFlags kUsage)
	: _size{ kSize }
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = kSize;
	bufferInfo.usage = kUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(LogicalDevice::Instance()._device, &bufferInfo, nullptr, &_buffer);
	check_vk_result(result);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(LogicalDevice::Instance()._device, _buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(kDevice._memoryProperties, memRequirements.memoryTypeBits,
												VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	result = vkAllocateMemory(LogicalDevice::Instance()._device, &allocInfo, nullptr, &_memory);
	check_vk_result(result);

	result = vkBindBufferMemory(LogicalDevice::Instance()._device, _buffer, _memory, 0);
	check_vk_result(result);
}

Buffer::~Buffer()
{
	if(_memory != VK_NULL_HANDLE)
		vkFreeMemory(LogicalDevice::Instance()._device, _memory, Context::Instance()._allocator);
	if(_buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(LogicalDevice::Instance()._device, _buffer, Context::Instance()._allocator);
}

Buffer::Buffer(Buffer&& buffer)
	: _size{ buffer._size}, _buffer{ buffer._buffer }, _memory{ buffer._memory }
{
	buffer._size = 0;
	buffer._buffer = VK_NULL_HANDLE;
	buffer._memory = VK_NULL_HANDLE;
}

Buffer& Buffer::operator=(Buffer&& buffer)
{
	if (_memory != VK_NULL_HANDLE)
		vkFreeMemory(LogicalDevice::Instance()._device, _memory, Context::Instance()._allocator);
	if (_buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(LogicalDevice::Instance()._device, _buffer, Context::Instance()._allocator);

	_size = buffer._size;
	_buffer = buffer._buffer;
	_memory = buffer._memory;

	buffer._size = 0;
	buffer._buffer = VK_NULL_HANDLE;
	buffer._memory = VK_NULL_HANDLE;

	return *this;
}

void Buffer::Map(void* data, size_t size)
{
	void* memoryPtr = nullptr;
	vkMapMemory(LogicalDevice::Instance()._device, _memory, 0, _size, 0, &memoryPtr);
	memcpy(memoryPtr, data, size);
	vkUnmapMemory(LogicalDevice::Instance()._device, _memory);
}