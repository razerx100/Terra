#ifndef VK_EXTERNAL_RESOURCE_FACTORY_HPP_
#define VK_EXTERNAL_RESOURCE_FACTORY_HPP_
#include <utility>
#include <ExternalResourceFactory.hpp>
#include <VkAllocator.hpp>

class VkExternalResourceFactory : public ExternalResourceFactory
{
public:
	VkExternalResourceFactory(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager }
	{}

	[[nodiscard]]
	std::unique_ptr<ExternalBuffer> CreateExternalBuffer(ExternalBufferType type) const override;

private:
	VkDevice       m_device;
	MemoryManager* m_memoryManager;

public:
	VkExternalResourceFactory(const VkExternalResourceFactory& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager }
	{}
	VkExternalResourceFactory& operator=(const VkExternalResourceFactory& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;

		return *this;
	}

	VkExternalResourceFactory(VkExternalResourceFactory&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ std::exchange(other.m_memoryManager, nullptr) }
	{}
	VkExternalResourceFactory& operator=(VkExternalResourceFactory&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = std::exchange(other.m_memoryManager, nullptr);

		return *this;
	}
};
#endif
