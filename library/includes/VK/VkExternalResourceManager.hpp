#ifndef VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#define VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <vector>
#include <ExternalResourceManager.hpp>
#include <VkExternalResourceFactory.hpp>
#include <VkDescriptorBuffer.hpp>
#include <StagingBufferManager.hpp>

class VkExternalResourceManager : public ExternalResourceManager
{
	using GfxExtension = std::shared_ptr<GraphicsTechniqueExtension>;

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

	void UpdateExtensionData(size_t frameIndex) const noexcept;

	void SetGraphicsDescriptorLayout(std::vector<VkDescriptorBuffer>& descriptorBuffers);

	[[nodiscard]]
	ExternalResourceFactory* GetResourceFactory() noexcept override
	{
		return &m_resourceFactory;
	}
	[[nodiscard]]
	ExternalResourceFactory const* GetResourceFactory() const noexcept override
	{
		return &m_resourceFactory;
	}

private:
	void OnGfxExtensionAddition(GraphicsTechniqueExtension& gfxExtension);
	void OnGfxExtensionDeletion(const GraphicsTechniqueExtension& gfxExtension);

	void UpdateDescriptor(
		VkDescriptorBuffer& descriptorBuffer, const ExternalBufferBindingDetails& bindingDetails
	) const;

private:
	VkExternalResourceFactory m_resourceFactory;
	std::vector<GfxExtension> m_gfxExtensions;

	static constexpr std::uint32_t s_externalBufferSetLayoutIndex = 2u;

public:
	VkExternalResourceManager(const VkExternalResourceManager&) = delete;
	VkExternalResourceManager& operator=(const VkExternalResourceManager&) = delete;

	VkExternalResourceManager(VkExternalResourceManager&& other) noexcept
		: m_resourceFactory{ std::move(other.m_resourceFactory) },
		m_gfxExtensions{ std::move(other.m_gfxExtensions) }
	{}
	VkExternalResourceManager& operator=(VkExternalResourceManager&& other) noexcept
	{
		m_resourceFactory = std::move(other.m_resourceFactory);
		m_gfxExtensions   = std::move(other.m_gfxExtensions);

		return *this;
	}
};
#endif
