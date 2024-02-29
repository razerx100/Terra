#ifndef I_SURFACE_MANAGER_HPP_
#define I_SURFACE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkExtensionManager.hpp>

class SurfaceManager
{
public:
	SurfaceManager() : m_surface{ VK_NULL_HANDLE }, m_instance{ VK_NULL_HANDLE } {}
	virtual ~SurfaceManager() noexcept
	{
		SelfDestruct();
	}

	virtual void Create(VkInstance instance, void* windowHandle, void* moduleHandle) = 0;

	[[nodiscard]]
	VkSurfaceKHR Get() const noexcept { return m_surface; }
	[[nodiscard]]
	const std::vector<InstanceExtension>& GetRequiredExtensions() const noexcept
	{
		return m_requiredExtensions;
	}

private:
	void SelfDestruct() noexcept;

protected:
	VkSurfaceKHR m_surface;
	VkInstance   m_instance;

	std::vector<InstanceExtension> m_requiredExtensions
	{ InstanceExtension::VkKhrSurface };

public:
	SurfaceManager(const SurfaceManager&) = delete;
	SurfaceManager& operator=(const SurfaceManager&) = delete;

	SurfaceManager(SurfaceManager&& other) noexcept
		: m_surface{ other.m_surface }, m_instance{ other.m_instance }
	{
		other.m_surface = VK_NULL_HANDLE;
	}
	SurfaceManager& operator=(SurfaceManager&& other) noexcept
	{
		SelfDestruct();

		m_surface       = other.m_surface;
		m_instance      = other.m_instance;
		other.m_surface = VK_NULL_HANDLE;

		return *this;
	}
};
#endif
