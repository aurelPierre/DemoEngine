#pragma once

#include <vulkan/vulkan.h>

#include "Device.h"

class CommandBuffer
{
public:
	const VkCommandPool _commandPool	= VK_NULL_HANDLE;
	VkCommandBuffer		_commandBuffer	= VK_NULL_HANDLE;

public:
	CommandBuffer() = default;
	CommandBuffer(const Queue& kQueue, const VkCommandBufferLevel kLevel = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	~CommandBuffer();

	CommandBuffer(const CommandBuffer& kCommandBuffer) = delete;
	CommandBuffer(CommandBuffer&& commandBuffer);

	CommandBuffer& operator=(const CommandBuffer& kCommandBuffer) = delete;
	CommandBuffer& operator=(CommandBuffer&& commandBuffer);

public:
	static CommandBuffer BeginSingleTimeCommands(const Queue& kQueue);
	static void EndSingleTimeCommands(const Queue& kQueue, const CommandBuffer& kCommandBuffer);

private:
	void Clean();

public:
	void Begin(const VkCommandBufferUsageFlags kUsage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) const;
	void End() const;
};