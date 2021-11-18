#ifndef __COMMAND_POOL_MANAGER_HPP__
#define __COMMAND_POOL_MANAGER_HPP__
#include <ICommandPoolManager.hpp>
#include <vector>

class CommandPoolManager : public ICommandPoolManager {
public:
	CommandPoolManager(
		VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount
	);
	~CommandPoolManager() noexcept;

	void Reset(std::uint32_t allocIndex) override;
	void Close() override;
	VkCommandBuffer GetCommandBuffer() const noexcept override;

private:
	VkDevice m_deviceRef;
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;
	VkCommandBufferBeginInfo m_beginInfo;
	std::uint32_t m_currentBufferIndex;
};
#endif
