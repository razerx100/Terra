#ifndef SURFACE_MANAGER_HPP_
#define SURFACE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <utility>
#include <VkExtensionManager.hpp>

class SurfaceInstanceExtension
{
public:
	virtual ~SurfaceInstanceExtension() = default;

	virtual void SetInstanceExtensions(VkInstanceExtensionManager& extensionManager) noexcept;
};

class SurfaceManager
{
public:
	SurfaceManager() : m_surface{ VK_NULL_HANDLE }, m_instance{ VK_NULL_HANDLE } {}
	virtual ~SurfaceManager() noexcept;

	virtual void Create(VkInstance instance, void* windowHandle, void* moduleHandle) = 0;

	[[nodiscard]]
	VkPresentModeKHR GetPresentMode(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	VkSurfaceFormatKHR GetSurfaceFormat(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	bool CanDeviceSupportSurface(VkPhysicalDevice device) const noexcept;

	[[nodiscard]]
	VkSurfaceKHR Get() const noexcept { return m_surface; }

private:
	void SelfDestruct() noexcept;

protected:
	VkSurfaceKHR m_surface;
	VkInstance   m_instance;

public:
	SurfaceManager(const SurfaceManager&) = delete;
	SurfaceManager& operator=(const SurfaceManager&) = delete;

	SurfaceManager(SurfaceManager&& other) noexcept
		: m_surface{ std::exchange(other.m_surface, VK_NULL_HANDLE) }, m_instance{ other.m_instance }
	{}
	SurfaceManager& operator=(SurfaceManager&& other) noexcept
	{
		SelfDestruct();

		m_surface  = std::exchange(other.m_surface, VK_NULL_HANDLE);
		m_instance = other.m_instance;

		return *this;
	}
};
#endif
