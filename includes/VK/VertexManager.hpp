#ifndef VERTEX_MANAGER_HPP_
#define VERTEX_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>

class VertexManager {
public:
	virtual ~VertexManager() = default;

	virtual void AddGlobalVertices(
		VkDevice device, std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept = 0;

	virtual void BindVertices(VkCommandBuffer graphicsCmdBuffer) const noexcept = 0;

	virtual void AcquireOwnerShips(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept = 0;
	virtual void ReleaseOwnerships(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept = 0;
	virtual void RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept = 0;
	virtual void ReleaseUploadResources() noexcept = 0;
	virtual void BindResourceToMemory(VkDevice device) const noexcept = 0;
};
#endif
