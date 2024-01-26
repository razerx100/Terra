#ifndef VERTEX_MANAGER_VERTEX_SHADER_HPP_
#define VERTEX_MANAGER_VERTEX_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <VkResourceViews.hpp>

#include <IModel.hpp>

class VertexManagerVertexShader {
public:
	VertexManagerVertexShader(VkDevice device) noexcept;

	void AddGVerticesAndIndices(
		VkDevice device, std::vector<Vertex>&& gVertices, std::vector<std::uint32_t>&& gIndices
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
	std::vector<Vertex> m_gVertices;
	std::vector<std::uint32_t> m_gIndices;
};
#endif
