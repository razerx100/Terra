#ifndef VERTEX_MANAGER_VERTEX_SHADER_HPP_
#define VERTEX_MANAGER_VERTEX_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <VkResourceViews.hpp>

class VertexManagerVertexShader {
public:
	VertexManagerVertexShader(VkDevice device) noexcept;

	void AddGlobalVertices(
		VkDevice device, std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept;

	void BindVertexAndIndexBuffer(VkCommandBuffer graphicsCmdBuffer) const noexcept;

	void AcquireOwnerShips(
		VkCommandBuffer graphicsCmdBuffer, std::uint32_t srcQueueIndex,
		std::uint32_t dstQueueIndex
	) noexcept;
	void ReleaseOwnerships(
		VkCommandBuffer transferCmdBuffer, std::uint32_t srcQueueIndex,
		std::uint32_t dstQueueIndex
	) noexcept;
	void RecordCopy(VkCommandBuffer transferCmdBuffer) noexcept;
	void ReleaseUploadResources() noexcept;
	void BindResourceToMemory(VkDevice device) const noexcept;

private:
	VkUploadableBufferResourceView m_gVertexBuffer;
	VkUploadableBufferResourceView m_gIndexBuffer;
};
#endif
