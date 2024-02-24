#ifndef SURFACE_MANAGER_WIN32_HPP_
#define SURFACE_MANAGER_WIN32_HPP_
#include <ISurfaceManager.hpp>

class SurfaceManagerWin32 final : public ISurfaceManager
{
public:
	SurfaceManagerWin32();
	~SurfaceManagerWin32() noexcept override;

	void Create(VkInstance instance, void* windowHandle, void* moduleHandle) override;

	[[nodiscard]]
	VkSurfaceKHR Get() const noexcept override { return m_surface; }

private:
	VkSurfaceKHR m_surface;
	VkInstance   m_instance;

	inline static const std::vector<InstanceExtension> s_requiredExtensions
	{
		InstanceExtension::VkKhrSurface,
		InstanceExtension::VkKhrWin32Surface
	};

public:
	[[nodiscard]]
	const std::vector<InstanceExtension>& GetRequiredExtensions() const noexcept override
	{ return s_requiredExtensions; }

	SurfaceManagerWin32(const SurfaceManagerWin32&) = delete;
	SurfaceManagerWin32& operator=(const SurfaceManagerWin32&) = delete;

	SurfaceManagerWin32(SurfaceManagerWin32&& other) noexcept
		: m_surface{ other.m_surface }, m_instance{ other.m_instance }
	{
		other.m_surface = VK_NULL_HANDLE;
	}
	SurfaceManagerWin32& operator=(SurfaceManagerWin32&& other) noexcept
	{
		m_surface       = other.m_surface;
		m_instance      = other.m_instance;
		other.m_surface = VK_NULL_HANDLE;

		return *this;
	}
};
#endif
