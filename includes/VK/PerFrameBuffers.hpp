#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <VkResourceViews.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers(
		VkDevice device, std::vector<std::uint32_t> queueFamilyIndices
	) noexcept;

	void BindPerFrameBuffers(VkCommandBuffer commandBuffer) const noexcept;

	void AddModelInputs(
		VkDevice device,
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);
	void BindResourceToMemory(
		VkDevice device, VkDeviceMemory uploadmemory, VkDeviceMemory gpuMemory
	);
	void RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept;
	void ReleaseUploadResources() noexcept;

private:
	void InitBuffers(VkDevice device) noexcept;
	void AddDescriptorForBuffer(
		VkBuffer buffer, size_t bufferSize,
		std::uint32_t bindingSlot, VkShaderStageFlagBits shaderStage
	) noexcept;

private:
	std::shared_ptr<VkResourceView> m_pCameraBuffer;
	VkUploadableBufferResourceView m_gVertexBuffer;
	VkUploadableBufferResourceView m_gIndexBuffer;
	std::vector<std::uint32_t> m_queueFamilyIndices;
};
#endif
