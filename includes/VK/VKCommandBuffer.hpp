#ifndef VK_COMMAND_BUFFER_HPP_
#define VK_COMMAND_BUFFER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class VKCommandBuffer {
public:
	VKCommandBuffer(
		VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount = 1u
	);
	~VKCommandBuffer() noexcept;

	void ResetBuffer(size_t index = 0u) const noexcept;
	void CloseBuffer(size_t index = 0u) const noexcept;

	[[nodiscard]]
	VkCommandBuffer GetCommandBuffer(size_t index = 0u) const noexcept;

private:
	VkDevice m_deviceRef;
	VkCommandPool m_commandPool;
	std::vector<VkCommandBuffer> m_commandBuffers;
};
#endif
