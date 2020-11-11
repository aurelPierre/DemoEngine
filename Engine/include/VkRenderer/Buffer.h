#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"

class Buffer
{
public:
	VkDeviceSize	_size	= 0;
	VkBuffer		_buffer = VK_NULL_HANDLE;
	VkDeviceMemory	_memory = VK_NULL_HANDLE;

public:
	Buffer() = default;
	Buffer(const VkDeviceSize kSize, const VkBufferUsageFlags kUsage);

	~Buffer();

	Buffer(const Buffer& kBuffer) = delete;
	Buffer(Buffer&& buffer);

	Buffer& operator=(const Buffer& kBuffer) = delete;
	Buffer& operator=(Buffer&& buffer);

private:
	void Clean();

public:
	void Map(void* data, size_t size) const;

	void CopyBuffer(const Queue& kQueue, const Buffer& kSrcBuffer) const;

public:
	operator const VkBuffer&() const;
};