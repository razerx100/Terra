#include <VKCommandBuffer.hpp>

VKCommandBuffer::VKCommandBuffer(VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount)
	: m_deviceRef{ device }, m_commandBuffers{ bufferCount, VK_NULL_HANDLE }
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	vkCreateCommandPool(m_deviceRef, &poolInfo, nullptr, &m_commandPool);

	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = m_commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = bufferCount;

	vkAllocateCommandBuffers(m_deviceRef, &allocateInfo, std::data(m_commandBuffers));
}

VKCommandBuffer::~VKCommandBuffer() noexcept {
	vkDestroyCommandPool(m_deviceRef, m_commandPool, nullptr);
}

void VKCommandBuffer::ResetBuffer(size_t index) const noexcept {
	static constexpr VkCommandBufferBeginInfo  beginInfo{
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		nullptr,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		nullptr
	};

	vkBeginCommandBuffer(m_commandBuffers[index], &beginInfo);
}

void VKCommandBuffer::CloseBuffer(size_t index) const noexcept {
	vkEndCommandBuffer(m_commandBuffers[index]);
}

void VKCommandBuffer::ResetFirstBuffer() const noexcept {
	ResetBuffer(0u);
}

void VKCommandBuffer::CloseFirstBuffer() const noexcept {
	CloseBuffer(0u);
}

VkCommandBuffer VKCommandBuffer::GetCommandBuffer(size_t index) const noexcept {
	return m_commandBuffers[index];
}

VkCommandBuffer VKCommandBuffer::GetFirstCommandBuffer() const noexcept {
	return GetCommandBuffer(0u);
}