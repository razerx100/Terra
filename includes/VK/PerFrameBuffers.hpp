#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <VkBuffers.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers(VkDevice device);

	void BindPerFrameBuffers(VkCommandBuffer commandBuffer) const noexcept;

	void AddModelInputs(
		VkDevice device,
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);

private:
	void InitBuffers(VkDevice device);
	void AddDescriptorForBuffer(
		VkBuffer buffer, size_t bufferSize,
		std::uint32_t bindingSlot, VkShaderStageFlagBits shaderStage
	);

private:
	std::shared_ptr<UploadBuffer> m_pCameraBuffer;
	std::shared_ptr<GpuBuffer> m_gVertexBuffer;
	std::shared_ptr<GpuBuffer> m_gIndexBuffer;
};
#endif
