#ifndef VK_SURFACE_MANAGER_HPP_
#define VK_SURFACE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <utility>
#include <VkExtensionManager.hpp>

namespace Terra
{
namespace SurfaceInstanceExtension
{
	void SetInstanceExtensions(VkInstanceExtensionManager& extensionManager) noexcept;
};

class SurfaceManager
{
public:
	SurfaceManager() : m_surface{ VK_NULL_HANDLE }, m_instance{ VK_NULL_HANDLE } {}
	~SurfaceManager() noexcept;

	[[nodiscard]]
	VkPresentModeKHR GetPresentMode(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	VkSurfaceFormatKHR GetSurfaceFormat(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	VkSurfaceCapabilitiesKHR GetSurfaceCapabilities(VkPhysicalDevice device) const noexcept;

	[[nodiscard]]
	static bool CanDeviceSupportSurface(VkSurfaceKHR surface, VkPhysicalDevice device) noexcept;

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
		: m_surface{ std::exchange(other.m_surface, VK_NULL_HANDLE) },
		m_instance{ other.m_instance }
	{}
	SurfaceManager& operator=(SurfaceManager&& other) noexcept
	{
		SelfDestruct();

		m_surface  = std::exchange(other.m_surface, VK_NULL_HANDLE);
		m_instance = other.m_instance;

		return *this;
	}
};
}
#endif
