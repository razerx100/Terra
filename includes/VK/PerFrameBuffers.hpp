#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <VkBuffers.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers(VkDevice device);

	void UpdatePerFrameBuffers() const noexcept;

private:
	void InitBuffers(VkDevice device);
	void AddDescriptorForBuffer(
		VkBuffer buffer, size_t bufferSize,
		std::uint32_t bindingSlot, VkShaderStageFlagBits shaderStage
	);

private:
	std::shared_ptr<UploadBuffer> m_pCameraBuffer;
};
#endif
