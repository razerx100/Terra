#ifndef MESH_MANAGER_MESH_SHADER_HPP_
#define MESH_MANAGER_MESH_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>

#include <MeshBundle.hpp>

class MeshManagerMeshShader
{
public:
	MeshManagerMeshShader(VkDevice device, MemoryManager* memoryManager);

	void SetMeshBundle(
		std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan
	);

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t verticesBindingSlot,
		std::uint32_t vertexIndicesBindingSlot, std::uint32_t primIndicesBindingSlot
	) const noexcept;

	void CleanupTempData() noexcept;

	[[nodiscard]]
	const std::vector<MeshBounds>& GetBounds() const noexcept { return m_meshBundle->GetBounds(); }

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
		const std::vector<T>& bufferData, Buffer& buffer, StagingBufferManager& stagingBufferMan
	) noexcept {
		const auto bufferSize = static_cast<VkDeviceSize>(sizeof(T) * std::size(bufferData));

		buffer.Create(
			bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
		);

		stagingBufferMan.AddBuffer(
			std::data(bufferData), bufferSize, &buffer, 0u,
			QueueType::GraphicsQueue, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT
		);
	}

private:
	[[nodiscard]]
	static std::vector<GLSLVertex> TransformVertices(const std::vector<Vertex>& vertices) noexcept;

private:
	Buffer                        m_vertexBuffer;
	Buffer                        m_vertexIndicesBuffer;
	Buffer                        m_primIndicesBuffer;
	std::vector<GLSLVertex>       m_vertices;
	std::unique_ptr<MeshBundleMS> m_meshBundle;

public:
	MeshManagerMeshShader(const MeshManagerMeshShader&) = delete;
	MeshManagerMeshShader& operator=(const MeshManagerMeshShader&) = delete;

	MeshManagerMeshShader(MeshManagerMeshShader&& other) noexcept
		: m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) },
		m_vertices{ std::move(other.m_vertices) },
		m_meshBundle{ std::move(other.m_meshBundle) }
	{}

	MeshManagerMeshShader& operator=(MeshManagerMeshShader&& other) noexcept
	{
		m_vertexBuffer        = std::move(other.m_vertexBuffer);
		m_vertexIndicesBuffer = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer   = std::move(other.m_primIndicesBuffer);
		m_vertices            = std::move(other.m_vertices);
		m_meshBundle          = std::move(other.m_meshBundle);

		return *this;
	}
};
#endif
