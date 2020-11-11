#include "VkRenderer/Buffer.h"

#include "VkRenderer/Core.h"
#include "VkRenderer/Context.h"
#include "VkRenderer/CommandBuffer.h"

Buffer::Buffer(const VkDeviceSize kSize, const VkBufferUsageFlags kUsage)
	: _size{ kSize }
{
	ASSERT(_size != 0u, "kSize is 0")

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = kSize;
	bufferInfo.usage = kUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(LogicalDevice::Instance()._device, &bufferInfo, Context::Instance()._allocator, &_buffer);
	check_vk_result(result);

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(LogicalDevice::Instance()._device, _buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = LogicalDevice::Instance()._physicalDevice->
									FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	result = vkAllocateMemory(LogicalDevice::Instance()._device, &allocInfo, Context::Instance()._allocator, &_memory);
	check_vk_result(result);

	result = vkBindBufferMemory(LogicalDevice::Instance()._device, _buffer, _memory, 0);
	check_vk_result(result);
}

Buffer::~Buffer()
{
	Clean();
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
	Clean();

	_size = buffer._size;
	_buffer = buffer._buffer;
	_memory = buffer._memory;

	buffer._size = 0;
	buffer._buffer = VK_NULL_HANDLE;
	buffer._memory = VK_NULL_HANDLE;

	return *this;
}

void Buffer::Clean()
{
	if (_memory != VK_NULL_HANDLE)
		vkFreeMemory(LogicalDevice::Instance()._device, _memory, Context::Instance()._allocator);
	if (_buffer != VK_NULL_HANDLE)
		vkDestroyBuffer(LogicalDevice::Instance()._device, _buffer, Context::Instance()._allocator);
}

void Buffer::Map(void* data, size_t size) const
{
	ASSERT(data != nullptr, "data is nullptr")
	ASSERT(size != 0u, "size is 0")

	void* memoryPtr = nullptr;
	vkMapMemory(LogicalDevice::Instance()._device, _memory, 0, _size, 0, &memoryPtr);
	memcpy(memoryPtr, data, size);
	vkUnmapMemory(LogicalDevice::Instance()._device, _memory);
}

void Buffer::CopyBuffer(const Queue& kQueue, const Buffer& kSrcBuffer) const
{
	ASSERT(kSrcBuffer._size == _size, "different size src : " + std::to_string(kSrcBuffer._size) + ", dst : " + std::to_string(_size))

	CommandBuffer commandBuffer = CommandBuffer::BeginSingleTimeCommands(kQueue);

	VkBufferCopy copyRegion{};
	copyRegion.size = _size;
	vkCmdCopyBuffer(commandBuffer, kSrcBuffer, _buffer, 1, &copyRegion);

	CommandBuffer::EndSingleTimeCommands(kQueue, commandBuffer);
}

const VkDescriptorBufferInfo Buffer::CreateDescriptorInfo() const
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = _buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = _size;

	return bufferInfo;
}

Buffer::operator const VkBuffer& () const
{
	return _buffer;
}
