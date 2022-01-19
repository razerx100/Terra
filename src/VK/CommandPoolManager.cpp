#include <CommandPoolManager.hpp>
#include <VKThrowMacros.hpp>
#include <ISwapChainManager.hpp>

CommandPoolManager::CommandPoolManager(
	VkDevice device, size_t queueIndex, size_t bufferCount
) : m_deviceRef(device), m_commandBuffers(bufferCount),
	m_beginInfo{} {

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = static_cast<std::uint32_t>(queueIndex);
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateCommandPool(device, &poolInfo, nullptr, &m_commandPool)
	);

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = m_commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = static_cast<std::uint32_t>(bufferCount);

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

void CommandPoolManager::Reset(size_t bufferIndex) {
	VkResult result;
	VK_THROW_FAILED(result,
		vkBeginCommandBuffer(m_commandBuffers[bufferIndex], &m_beginInfo)
	);
}

void CommandPoolManager::Close(size_t bufferIndex) {
	VkResult result;
	VK_THROW_FAILED(result,
		vkEndCommandBuffer(m_commandBuffers[bufferIndex])
	);
}

VkCommandBuffer CommandPoolManager::GetCommandBuffer(size_t bufferIndex) const noexcept {
	return m_commandBuffers[bufferIndex];
}
