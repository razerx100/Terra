#ifndef VK_EXTERNAL_RESOURCE_FACTORY_HPP_
#define VK_EXTERNAL_RESOURCE_FACTORY_HPP_
#include <utility>
#include <vector>
#include <ExternalResourceFactory.hpp>
#include <VkAllocator.hpp>
#include <VkExternalBuffer.hpp>
#include <ReusableVector.hpp>

class VkExternalResourceFactory : public ExternalResourceFactory
{
	using ExternalBuffer_t = std::shared_ptr<VkExternalBuffer>;

public:
	VkExternalResourceFactory(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager }, m_externalBuffers{}
	{}

	[[nodiscard]]
	size_t CreateExternalBuffer(ExternalBufferType type) override;
	[[nodiscard]]
	ExternalBuffer* GetExternalBufferRP(size_t index) const noexcept override
	{
		return m_externalBuffers[index].get();
	}
	[[nodiscard]]
	std::shared_ptr<ExternalBuffer> GetExternalBufferSP(size_t index) const noexcept override
	{
		return std::static_pointer_cast<ExternalBuffer>(m_externalBuffers[index]);
	}

	[[nodiscard]]
	VkExternalBuffer& GetVkExternalBuffer(size_t index) noexcept
	{
		return *m_externalBuffers[index];
	}
	[[nodiscard]]
	const VkExternalBuffer& GetVkExternalBuffer(size_t index) const noexcept
	{
		return *m_externalBuffers[index];
	}

	void RemoveExternalBuffer(size_t index) noexcept override;

private:
	VkDevice                         m_device;
	MemoryManager*                   m_memoryManager;
	ReusableVector<ExternalBuffer_t> m_externalBuffers;

public:
	VkExternalResourceFactory(const VkExternalResourceFactory&) = delete;
	VkExternalResourceFactory& operator=(const VkExternalResourceFactory&) = delete;

	VkExternalResourceFactory(VkExternalResourceFactory&& other) noexcept
		: m_device{ other.m_device },
		m_memoryManager{ std::exchange(other.m_memoryManager, nullptr) },
		m_externalBuffers{ std::move(other.m_externalBuffers) }
	{}
	VkExternalResourceFactory& operator=(VkExternalResourceFactory&& other) noexcept
	{
		m_device          = other.m_device;
		m_memoryManager   = std::exchange(other.m_memoryManager, nullptr);
		m_externalBuffers = std::move(other.m_externalBuffers);

		return *this;
	}
};
#endif
