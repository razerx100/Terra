#ifndef VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#define VK_EXTERNAL_RESOURCE_MANAGER_HPP_
#include <VkExternalResourceFactory.hpp>

class VkExternalResourceManager
{
public:
	VkExternalResourceManager(VkDevice device, MemoryManager* memoryManager);

	[[nodiscard]]
	ExternalResourceFactory const* GetResourceFactory() const noexcept { return &m_resourceFactory; }

private:
	VkExternalResourceFactory m_resourceFactory;

public:
	VkExternalResourceManager(const VkExternalResourceManager&) = delete;
	VkExternalResourceManager& operator=(const VkExternalResourceManager&) = delete;

	VkExternalResourceManager(VkExternalResourceManager&& other) noexcept
		: m_resourceFactory{ std::move(other.m_resourceFactory) }
	{}
	VkExternalResourceManager& operator=(VkExternalResourceManager&& other) noexcept
	{
		m_resourceFactory = std::move(other.m_resourceFactory);

		return *this;
	}
};
#endif
