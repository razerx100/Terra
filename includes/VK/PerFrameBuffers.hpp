#ifndef PER_FRAME_BUFFERS_HPP_
#define PER_FRAME_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <VkResourceViews.hpp>

class PerFrameBuffers {
public:
	PerFrameBuffers(
		VkDevice device, std::vector<std::uint32_t> queueFamilyIndices,
		std::uint32_t bufferCount
	) noexcept;

	void BindPerFrameBuffers(VkCommandBuffer commandBuffer, size_t frameIndex) const noexcept;

	void AddModelInputs(
		VkDevice device,
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);
	void BindResourceToMemory(VkDevice device);
	void RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept;
	void ReleaseUploadResources() noexcept;

private:
	void InitBuffers(VkDevice device, std::uint32_t bufferCount) noexcept;

private:
	VkResourceView m_cameraBuffer;
	VkUploadableBufferResourceView m_gVertexBuffer;
	VkUploadableBufferResourceView m_gIndexBuffer;
	std::vector<std::uint32_t> m_queueFamilyIndices;
};
#endif
