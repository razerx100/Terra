#ifndef VK_EXTERNAL_RESOURCE_FACTORY_HPP_
#define VK_EXTERNAL_RESOURCE_FACTORY_HPP_
#include <utility>
#include <vector>
#include <VkAllocator.hpp>
#include <VkExternalBuffer.hpp>
#include <ReusableVector.hpp>

namespace Terra
{
class VkExternalResourceFactory
{
	using ExternalBuffer_t  = std::shared_ptr<VkExternalBuffer>;
	using ExternalTexture_t = std::shared_ptr<VkExternalTexture>;

public:
	VkExternalResourceFactory(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager }, m_externalBuffers{},
		m_externalTextures{}
	{}

	[[nodiscard]]
	size_t CreateExternalBuffer(ExternalBufferType type);
	[[nodiscard]]
	size_t CreateExternalTexture();

	[[nodiscard]]
	VkExternalBuffer* GetExternalBufferRP(size_t index) const noexcept
	{
		return m_externalBuffers[index].get();
	}
	[[nodiscard]]
	VkExternalTexture* GetExternalTextureRP(size_t index) const noexcept
	{
		return m_externalTextures[index].get();
	}

	[[nodiscard]]
	std::shared_ptr<VkExternalBuffer> GetExternalBufferSP(size_t index) const noexcept
	{
		return m_externalBuffers[index];
	}
	[[nodiscard]]
	std::shared_ptr<VkExternalTexture> GetExternalTextureSP(size_t index) const noexcept
	{
		return m_externalTextures[index];
	}

	[[nodiscard]]
	const Buffer& GetVkBuffer(size_t index) const noexcept
	{
		return m_externalBuffers[index]->GetBuffer();
	}
	[[nodiscard]]
	const VkTextureView& GetVkTextureView(size_t index) const noexcept
	{
		return m_externalTextures[index]->GetTextureView();
	}

	void RemoveExternalBuffer(size_t index) noexcept
	{
		m_externalBuffers[index].reset();
		m_externalBuffers.RemoveElement(index);
	}
	void RemoveExternalTexture(size_t index) noexcept
	{
		m_externalTextures[index].reset();
		m_externalTextures.RemoveElement(index);
	}

private:
	VkDevice                                    m_device;
	MemoryManager*                              m_memoryManager;
	Callisto::ReusableVector<ExternalBuffer_t>  m_externalBuffers;
	Callisto::ReusableVector<ExternalTexture_t> m_externalTextures;

public:
	VkExternalResourceFactory(const VkExternalResourceFactory&) = delete;
	VkExternalResourceFactory& operator=(const VkExternalResourceFactory&) = delete;

	VkExternalResourceFactory(VkExternalResourceFactory&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ std::exchange(other.m_memoryManager, nullptr) },
		m_externalBuffers{ std::move(other.m_externalBuffers) },
		m_externalTextures{ std::move(other.m_externalTextures) }
	{}
	VkExternalResourceFactory& operator=(VkExternalResourceFactory&& other) noexcept
	{
		m_device           = other.m_device;
		m_memoryManager    = std::exchange(other.m_memoryManager, nullptr);
		m_externalBuffers  = std::move(other.m_externalBuffers);
		m_externalTextures = std::move(other.m_externalTextures);

		return *this;
	}
};
}
#endif
