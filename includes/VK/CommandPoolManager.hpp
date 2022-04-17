#ifndef COMMAND_POOL_MANAGER_HPP_
#define COMMAND_POOL_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class CommandPoolManager {
public:
	CommandPoolManager(
		VkDevice device, size_t queueIndex, std::uint32_t bufferCount
	);
	~CommandPoolManager() noexcept;

	void Reset(size_t bufferIndex);
	void Close(size_t bufferIndex);

	[[nodiscard]]
	VkCommandBuffer GetCommandBuffer(size_t bufferIndex) const noexcept;

private:
	VkDevice m_deviceRef;
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;
	VkCommandBufferBeginInfo m_beginInfo;
};
#endif
