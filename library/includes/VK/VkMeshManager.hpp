#ifndef VK_MESH_MANAGER_HPP_
#define VK_MESH_MANAGER_HPP_
#include <memory>
#include <VkMeshBundleMS.hpp>
#include <VkMeshBundleVS.hpp>
#include <ReusableVector.hpp>
#include <VkSharedBuffers.hpp>
#include <VkCommandQueue.hpp>
#include <VkStagingBufferManager.hpp>

namespace Terra
{
template<class Derived, class VkMeshBundle>
class MeshManager
{
public:
	MeshManager() : m_meshBundles{}, m_oldBufferCopyNecessary{ false } {}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(
		MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	) {
		VkMeshBundle vkMeshBundle{};

		static_cast<Derived*>(this)->ConfigureMeshBundle(
			std::move(meshBundle), stagingBufferMan, vkMeshBundle, tempBuffer
		);

		const size_t meshIndex   = m_meshBundles.Add(std::move(vkMeshBundle));

		m_oldBufferCopyNecessary = true;

		return static_cast<std::uint32_t>(meshIndex);
	}

	void RemoveMeshBundle(std::uint32_t bundleIndex) noexcept
	{
		static_cast<Derived*>(this)->ConfigureRemoveMesh(bundleIndex);

		// It is okay to use the non-clear function based RemoveElement, as I will be
		// moving the Buffers out as SharedBuffer.
		m_meshBundles.RemoveElement(bundleIndex);
	}

	[[nodiscard]]
	VkMeshBundle& GetBundle(size_t index) noexcept { return m_meshBundles.at(index); }
	[[nodiscard]]
	const VkMeshBundle& GetBundle(size_t index) const noexcept { return m_meshBundles.at(index); }

protected:
	Callisto::ReusableVector<VkMeshBundle> m_meshBundles;
	bool                                   m_oldBufferCopyNecessary;

public:
	MeshManager(const MeshManager&) = delete;
	MeshManager& operator=(const MeshManager&) = delete;

	MeshManager(MeshManager&& other) noexcept
		: m_meshBundles{ std::move(other.m_meshBundles) },
		m_oldBufferCopyNecessary{ other.m_oldBufferCopyNecessary }
	{}
	MeshManager& operator=(MeshManager&& other) noexcept
	{
		m_meshBundles            = std::move(other.m_meshBundles);
		m_oldBufferCopyNecessary = other.m_oldBufferCopyNecessary;

		return *this;
	}
};

class MeshManagerVSIndividual : public MeshManager<MeshManagerVSIndividual, VkMeshBundleVS>
{
	friend class MeshManager<MeshManagerVSIndividual, VkMeshBundleVS>;
public:
	MeshManagerVSIndividual(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3
	);

	void CopyOldBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept;

private:
	void ConfigureMeshBundle(
		MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
		VkMeshBundleVS& vkMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;

private:
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_indexBuffer;

public:
	MeshManagerVSIndividual(const MeshManagerVSIndividual&) = delete;
	MeshManagerVSIndividual& operator=(const MeshManagerVSIndividual&) = delete;

	MeshManagerVSIndividual(MeshManagerVSIndividual&& other) noexcept
		: MeshManager{ std::move(other) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) }
	{}

	MeshManagerVSIndividual& operator=(MeshManagerVSIndividual&& other) noexcept
	{
		MeshManager::operator=(std::move(other));
		m_vertexBuffer       = std::move(other.m_vertexBuffer);
		m_indexBuffer        = std::move(other.m_indexBuffer);

		return *this;
	}
};

class MeshManagerVSIndirect : public MeshManager<MeshManagerVSIndirect, VkMeshBundleVS>
{
	friend class MeshManager<MeshManagerVSIndirect, VkMeshBundleVS>;
public:
	MeshManagerVSIndirect(
		VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3
	);

	void CopyOldBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept;

	void SetDescriptorBufferLayoutCS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
	) const noexcept;

	void SetDescriptorBuffersCS(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
	) const;

private:
	void ConfigureMeshBundle(
		MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
		VkMeshBundleVS& vkMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;

private:
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_indexBuffer;
	SharedBufferGPU m_perMeshDataBuffer;
	SharedBufferGPU m_perMeshBundleDataBuffer;

	// Compute Shader
	static constexpr std::uint32_t s_perMeshDataBindingSlot       = 6u;
	static constexpr std::uint32_t s_perMeshBundleDataBindingSlot = 7u;

public:
	MeshManagerVSIndirect(const MeshManagerVSIndirect&) = delete;
	MeshManagerVSIndirect& operator=(const MeshManagerVSIndirect&) = delete;

	MeshManagerVSIndirect(MeshManagerVSIndirect&& other) noexcept
		: MeshManager{ std::move(other) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_indexBuffer{ std::move(other.m_indexBuffer) },
		m_perMeshDataBuffer{ std::move(other.m_perMeshDataBuffer) },
		m_perMeshBundleDataBuffer{ std::move(other.m_perMeshBundleDataBuffer) }
	{}
	MeshManagerVSIndirect& operator=(MeshManagerVSIndirect&& other) noexcept
	{
		MeshManager::operator=(std::move(other));
		m_vertexBuffer            = std::move(other.m_vertexBuffer);
		m_indexBuffer             = std::move(other.m_indexBuffer);
		m_perMeshDataBuffer       = std::move(other.m_perMeshDataBuffer);
		m_perMeshBundleDataBuffer = std::move(other.m_perMeshBundleDataBuffer);

		return *this;
	}
};

class MeshManagerMS : public MeshManager<MeshManagerMS, VkMeshBundleMS>
{
	friend class MeshManager<MeshManagerMS, VkMeshBundleMS>;
public:
	MeshManagerMS(VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3);

	void SetDescriptorBufferLayout(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex
	) const noexcept;

	// Should be called after a new Mesh has been added.
	void SetDescriptorBuffers(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex
	) const;

	void CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept;

private:
	void ConfigureMeshBundle(
		MeshBundleTemporaryData&& meshBundle, StagingBufferManager& stagingBufferMan,
		VkMeshBundleMS& vkMeshBundle, Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	void ConfigureRemoveMesh(size_t bundleIndex) noexcept;

private:
	SharedBufferGPU m_perMeshletDataBuffer;
	SharedBufferGPU m_vertexBuffer;
	SharedBufferGPU m_vertexIndicesBuffer;
	SharedBufferGPU m_primIndicesBuffer;

	static constexpr std::uint32_t s_perMeshletBufferBindingSlot    = 2u;
	static constexpr std::uint32_t s_vertexBufferBindingSlot        = 3u;
	static constexpr std::uint32_t s_vertexIndicesBufferBindingSlot = 4u;
	static constexpr std::uint32_t s_primIndicesBufferBindingSlot   = 5u;

public:
	MeshManagerMS(const MeshManagerMS&) = delete;
	MeshManagerMS& operator=(const MeshManagerMS&) = delete;

	MeshManagerMS(MeshManagerMS&& other) noexcept
		: MeshManager{ std::move(other) },
		m_perMeshletDataBuffer{ std::move(other.m_perMeshletDataBuffer) },
		m_vertexBuffer{ std::move(other.m_vertexBuffer) },
		m_vertexIndicesBuffer{ std::move(other.m_vertexIndicesBuffer) },
		m_primIndicesBuffer{ std::move(other.m_primIndicesBuffer) }
	{}
	MeshManagerMS& operator=(MeshManagerMS&& other) noexcept
	{
		MeshManager::operator=(std::move(other));
		m_perMeshletDataBuffer = std::move(other.m_perMeshletDataBuffer);
		m_vertexBuffer         = std::move(other.m_vertexBuffer);
		m_vertexIndicesBuffer  = std::move(other.m_vertexIndicesBuffer);
		m_primIndicesBuffer    = std::move(other.m_primIndicesBuffer);

		return *this;
	}
};
}
#endif
