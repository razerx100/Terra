#ifndef VERTEX_MANAGER_VERTEX_HPP_
#define VERTEX_MANAGER_VERTEX_HPP_
#include <VertexManager.hpp>
#include <VkResourceViews.hpp>
#include <optional>

class VertexManagerVertex : public VertexManager {
public:
	struct Args {
		std::optional<VkDevice> device;
	};

public:
	VertexManagerVertex(const Args& arguments);

	void AddGlobalVertices(
		VkDevice device, std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	) noexcept override;

	void BindVertices(VkCommandBuffer graphicsCmdBuffer) const noexcept override;

	void AcquireOwnerShips(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept override;
	void ReleaseOwnerships(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept override;
	void RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept override;
	void ReleaseUploadResources() noexcept override;
	void BindResourceToMemory(VkDevice device) const noexcept override;

private:
	VkUploadableBufferResourceView m_gVertexBuffer;
	VkUploadableBufferResourceView m_gIndexBuffer;
};
#endif
