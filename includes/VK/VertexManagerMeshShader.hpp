#ifndef VERTEX_MANAGER_MESH_SHADER_HPP_
#define VERTEX_MANAGER_MESH_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <VkResourceViews.hpp>
#include <VkQueueFamilyManager.hpp>
#include <UploadContainer.hpp>

#include <IModel.hpp>

class VertexManagerMeshShader {
public:
	VertexManagerMeshShader(
		VkDevice device, std::uint32_t bufferCount, QueueIndicesTG queueIndices
	) noexcept;

	void AddGVerticesAndPrimIndices(
		VkDevice device, std::vector<Vertex>&& gVertices,
		std::vector<std::uint32_t>&& gVerticesIndices, std::vector<std::uint32_t>&& gPrimIndices
	) noexcept;

	void AcquireOwnerShips(VkCommandBuffer graphicsCmdBuffer) noexcept;
	void ReleaseOwnerships(VkCommandBuffer transferCmdBuffer) noexcept;
	void RecordCopy(VkCommandBuffer transferCmdBuffer) noexcept;
	void ReleaseUploadResources() noexcept;
	void BindResourceToMemory(VkDevice device) const noexcept;

private:
	struct GLSLVertex {
		DirectX::XMFLOAT3 position;
		float padding0;
		DirectX::XMFLOAT3 normal;
		float padding1;
		DirectX::XMFLOAT2 uv;
		float padding3[2];
	};

private:
	void AddDescriptors(
		VkUploadableBufferResourceView& buffer, std::uint32_t bindingSlot
	) const noexcept;

	[[nodiscard]]
	static std::vector<GLSLVertex> TransformVertices(
		const std::vector<Vertex>& vertices
	) noexcept;

	template<typename T>
	static void ConfigureBuffer(
		VkDevice device, std::vector<T>&& input, std::vector<T>& output,
		VkUploadableBufferResourceView& buffer
	) noexcept {
		const size_t bufferSize = sizeof(T) * std::size(input);

		buffer.CreateResource(
			device, static_cast<VkDeviceSize>(bufferSize), 1u,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		);

		buffer.SetMemoryOffsetAndType(device);

		GetUploadContainer()->AddMemory(
			std::data(input), bufferSize, buffer.GetFirstUploadMemoryOffset()
		);

		output = std::move(input);
	}

	[[nodiscard]]
	static UploadContainer* GetUploadContainer() noexcept;

private:
	std::uint32_t m_bufferCount;
	QueueIndicesTG m_queueIndices;
	VkUploadableBufferResourceView m_vertexBuffer;
	VkUploadableBufferResourceView m_vertexIndicesBuffer;
	VkUploadableBufferResourceView m_primIndicesBuffer;
	std::vector<GLSLVertex> m_gVertices;
	std::vector<std::uint32_t> m_gVerticesIndices;
	std::vector<std::uint32_t> m_gPrimIndices;
};
#endif
