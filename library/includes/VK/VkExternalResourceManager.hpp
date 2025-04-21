#ifndef VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#define VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <vector>
#include <ExternalResourceManager.hpp>
#include <VkExternalResourceFactory.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>
#include <ReusableVector.hpp>
#include <VkCommandQueue.hpp>

class VkExternalResourceManager : public ExternalResourceManager
{
	using GfxExtension_t = std::shared_ptr<GraphicsTechniqueExtension>;

public:
	VkExternalResourceManager(VkDevice device, MemoryManager* memoryManager);

	[[nodiscard]]
	std::uint32_t AddGraphicsTechniqueExtension(
		std::shared_ptr<GraphicsTechniqueExtension> extension
	) override;

	void RemoveGraphicsTechniqueExtension(std::uint32_t index) noexcept override;

	void UpdateDescriptor(
		std::vector<VkDescriptorBuffer>& descriptorBuffers,
		const ExternalBufferBindingDetails& bindingDetails
	) const;

	void UploadExternalBufferGPUOnlyData(
		StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempGPUBuffer,
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	) const;

	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes,
		TemporaryDataBufferGPU& tempGPUBuffer
	);

	void CopyQueuedBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept;

	void UpdateExtensionData(size_t frameIndex) const noexcept;

	void SetGraphicsDescriptorLayout(std::vector<VkDescriptorBuffer>& descriptorBuffers);

	[[nodiscard]]
	ExternalResourceFactory* GetResourceFactory() noexcept override
	{
		return GetVkResourceFactory();
	}
	[[nodiscard]]
	ExternalResourceFactory const* GetResourceFactory() const noexcept override
	{
		return GetVkResourceFactory();
	}

	[[nodiscard]]
	VkExternalResourceFactory* GetVkResourceFactory() noexcept
	{
		return m_resourceFactory.get();
	}
	[[nodiscard]]
	VkExternalResourceFactory const* GetVkResourceFactory() const noexcept
	{
		return m_resourceFactory.get();
	}

private:
	struct GPUCopyDetails
	{
		std::uint32_t srcIndex;
		std::uint32_t dstIndex;
		VkDeviceSize  srcOffset;
		VkDeviceSize  srcSize;
		VkDeviceSize  dstOffset;
	};

private:
	void OnGfxExtensionDeletion(const GraphicsTechniqueExtension& gfxExtension);

	void UpdateDescriptor(
		VkDescriptorBuffer& descriptorBuffer, const ExternalBufferBindingDetails& bindingDetails
	) const;

private:
	std::unique_ptr<VkExternalResourceFactory> m_resourceFactory;
	Callisto::ReusableVector<GfxExtension_t>   m_gfxExtensions;
	std::vector<GPUCopyDetails>                m_copyQueueDetails;

	static constexpr std::uint32_t s_externalBufferSetLayoutIndex = 2u;

public:
	VkExternalResourceManager(const VkExternalResourceManager&) = delete;
	VkExternalResourceManager& operator=(const VkExternalResourceManager&) = delete;

	VkExternalResourceManager(VkExternalResourceManager&& other) noexcept
		: m_resourceFactory{ std::move(other.m_resourceFactory) },
		m_gfxExtensions{ std::move(other.m_gfxExtensions) },
		m_copyQueueDetails{ std::move(other.m_copyQueueDetails) }
	{}
	VkExternalResourceManager& operator=(VkExternalResourceManager&& other) noexcept
	{
		m_resourceFactory  = std::move(other.m_resourceFactory);
		m_gfxExtensions    = std::move(other.m_gfxExtensions);
		m_copyQueueDetails = std::move(other.m_copyQueueDetails);

		return *this;
	}
};
#endif
