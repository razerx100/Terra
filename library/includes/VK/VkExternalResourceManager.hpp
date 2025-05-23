#ifndef VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#define VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <vector>
#include <GraphicsTechniqueExtension.hpp>
#include <VkExternalResourceFactory.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkStagingBufferManager.hpp>
#include <ReusableVector.hpp>
#include <VkCommandQueue.hpp>

namespace Terra
{
class VkExternalResourceManager
{
	using GfxExtensionContainer_t = Callisto::ReusableVector<GraphicsTechniqueExtension>;

public:
	VkExternalResourceManager(VkDevice device, MemoryManager* memoryManager);

	[[nodiscard]]
	std::uint32_t AddGraphicsTechniqueExtension(GraphicsTechniqueExtension&& extension);

	void UpdateGraphicsTechniqueExtension(size_t index, GraphicsTechniqueExtension&& extension);

	void RemoveGraphicsTechniqueExtension(std::uint32_t index) noexcept;

	void UpdateDescriptor(
		std::vector<VkDescriptorBuffer>& descriptorBuffers,
		const ExternalBufferBindingDetails& bindingDetails
	) const;

	void UploadExternalBufferGPUOnlyData(
		StagingBufferManager& stagingBufferManager, Callisto::TemporaryDataBufferGPU& tempGPUBuffer,
		std::uint32_t externalBufferIndex, std::shared_ptr<void> cpuData, size_t srcDataSizeInBytes,
		size_t dstBufferOffset
	) const;

	void QueueExternalBufferGPUCopy(
		std::uint32_t externalBufferSrcIndex, std::uint32_t externalBufferDstIndex,
		size_t dstBufferOffset, size_t srcBufferOffset, size_t srcDataSizeInBytes,
		Callisto::TemporaryDataBufferGPU& tempGPUBuffer
	);

	void CopyQueuedBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept;

	void SetGraphicsDescriptorLayout(std::vector<VkDescriptorBuffer>& descriptorBuffers);

	[[nodiscard]]
	auto&& GetResourceFactory(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_resourceFactory);
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
	VkExternalResourceFactory   m_resourceFactory;
	GfxExtensionContainer_t     m_gfxExtensions;
	std::vector<GPUCopyDetails> m_copyQueueDetails;

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
}
#endif
