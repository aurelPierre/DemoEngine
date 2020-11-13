#include "VkRenderer/CommandBuffer.h"

#include "VkRenderer/Core.h"

CommandBuffer::CommandBuffer(const Queue& kQueue, const VkCommandBufferLevel kLevel)
	: _commandPool { kQueue._commandPool }
{
	VkCommandBufferAllocateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = _commandPool;
	info.level = kLevel;
	info.commandBufferCount = 1;

	VkResult err = vkAllocateCommandBuffers(LogicalDevice::Instance()._device, &info, &_commandBuffer);
	VK_ASSERT(err, "error when allocating command buffer");
}

CommandBuffer::~CommandBuffer()
{
	Clean();
}

CommandBuffer::CommandBuffer(CommandBuffer&& commandBuffer)
	: _commandBuffer { commandBuffer._commandBuffer }
{
	commandBuffer._commandBuffer = VK_NULL_HANDLE;
}

CommandBuffer& CommandBuffer::operator=(CommandBuffer&& commandBuffer)
{
	Clean();

	_commandBuffer = commandBuffer._commandBuffer;
	commandBuffer._commandBuffer = VK_NULL_HANDLE;

	return *this;
}

CommandBuffer CommandBuffer::BeginSingleTimeCommands(const Queue& kQueue)
{
	CommandBuffer commandBuffer(kQueue);

	commandBuffer.Begin();

	return commandBuffer;
}

void CommandBuffer::EndSingleTimeCommands(const Queue& kQueue, const CommandBuffer& kCommandBuffer)
{
	kCommandBuffer.End();

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &kCommandBuffer._commandBuffer;

	VkResult err = vkQueueSubmit(kQueue._queue, 1, &submitInfo, VK_NULL_HANDLE);
	VK_ASSERT(err, "error when submitting queue");

	err = vkQueueWaitIdle(kQueue._queue);
	VK_ASSERT(err, "error when waiting idle queue");
}

void CommandBuffer::Clean()
{
	if (_commandBuffer != VK_NULL_HANDLE)
		vkFreeCommandBuffers(LogicalDevice::Instance()._device, _commandPool, 1, &_commandBuffer);
}

void CommandBuffer::Begin(const VkCommandBufferUsageFlags kUsage) const
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = kUsage;

	VkResult err = vkBeginCommandBuffer(_commandBuffer, &beginInfo);
	VK_ASSERT(err, "error when beginning command buffer");
}

void CommandBuffer::End() const
{
	VkResult err = vkEndCommandBuffer(_commandBuffer);
	VK_ASSERT(err, "error when ending command buffer");
}

CommandBuffer::operator const VkCommandBuffer& () const
{
	return _commandBuffer;
}
