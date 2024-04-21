#ifndef VERTEX_MANAGER_MESH_SHADER_HPP_
#define VERTEX_MANAGER_MESH_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>

#include <IModel.hpp>

class VertexManagerMeshShader
{
public:
	VertexManagerMeshShader(VkDevice device, MemoryManager* memoryManager);

	void SetVerticesAndPrimIndices(
		std::vector<Vertex>&& vertices,
		std::vector<std::uint32_t>&& vertexIndices, std::vector<std::uint32_t>&& primIndices,
		StagingBufferManager& stagingBufferMan
	);

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t verticesBindingSlot,
		std::uint32_t vertexIndicesBindingSlot, std::uint32_t primIndicesBindingSlot
	) const noexcept;

	void CleanupTempData() noexcept;

private:
	struct GLSLVertex
	{
		DirectX::XMFLOAT3 position;
		float padding0;
		DirectX::XMFLOAT3 normal;
		float padding1;
		DirectX::XMFLOAT2 uv;
		float padding3[2];
	};

	template<typename T>
	static void ConfigureBuffer(
		std::vector<T>&& inputData, std::vector<T>& outputData, Buffer& buffer,
		StagingBufferManager& stagingBufferMan
	) noexcept {
		const auto bufferSize = static_cast<VkDeviceSize>(sizeof(T) * std::size(inputData));

		buffer.Create(
			bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
		);

		outputData = std::move(inputData);

		stagingBufferMan.AddBuffer(
			std::data(outputData), bufferSize, buffer, 0u,
			QueueType::GraphicsQueue, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT
		);
	}

private:
	[[nodiscard]]
	static std::vector<GLSLVertex> TransformVertices(const std::vector<Vertex>& vertices) noexcept;

private:
	Buffer                     m_vertexBuffer;
	Buffer                     m_vertexIndicesBuffer;
	Buffer                     m_primIndicesBuffer;
	std::vector<GLSLVertex>    m_vertices;
	std::vector<std::uint32_t> m_vertexIndices;
	std::vector<std::uint32_t> m_primIndices;

public:
	VertexManagerMeshShader(const VertexManagerMeshShader&) = delete;
	VertexManagerMeshShader& operator=(const VertexManagerMeshShader&) = delete;

	VertexManagerMeshShader(VertexManagerMeshShader&& other) noexcept
		: m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) },
		m_vertices{ std::move(other.m_vertices) }, m_vertexIndices{ std::move(other.m_vertexIndices) },
		m_primIndices{ std::move(other.m_primIndices) }
	{}

	VertexManagerMeshShader& operator=(VertexManagerMeshShader&& other) noexcept
	{
		m_vertexBuffer        = std::move(other.m_vertexBuffer);
		m_vertexIndicesBuffer = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer   = std::move(other.m_primIndicesBuffer);
		m_vertices            = std::move(other.m_vertices);
		m_vertexIndices       = std::move(other.m_vertexIndices);
		m_primIndices         = std::move(other.m_primIndices);

		return *this;
	}
};
#endif
