#include <CommandPoolManager.hpp>
#include <VKThrowMacros.hpp>
#include <ISwapChainManager.hpp>

CommandPoolManager::CommandPoolManager(
	VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount
) : m_deviceRef(device), m_commandBuffers(bufferCount),
	m_beginInfo{}, m_currentBufferIndex(0u) {

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueIndex;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool)
	);

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = m_commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = bufferCount;

	VK_THROW_FAILED(result,
		vkAllocateCommandBuffers(device, &allocateInfo, m_commandBuffers.data())
	);

	m_beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	m_beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	m_beginInfo.pInheritanceInfo = nullptr;
}

CommandPoolManager::~CommandPoolManager() noexcept {
	vkDestroyCommandPool(m_deviceRef, m_commandPool, nullptr);
}

void CommandPoolManager::Reset(std::uint32_t allocIndex) {
	m_currentBufferIndex = allocIndex;

	VkResult result;
	VK_THROW_FAILED(result,
		vkBeginCommandBuffer(m_commandBuffers[m_currentBufferIndex], &m_beginInfo)
	);
}

void CommandPoolManager::Close() {
	VkResult result;
	VK_THROW_FAILED(result,
		vkEndCommandBuffer(m_commandBuffers[m_currentBufferIndex])
	);
}

VkCommandBuffer CommandPoolManager::GetCommandBuffer() const noexcept {
	return m_commandBuffers[m_currentBufferIndex];
}
