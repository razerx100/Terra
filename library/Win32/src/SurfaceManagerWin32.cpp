#include <SurfaceManagerWin32.hpp>
#include <CleanWin.hpp>
#include <vulkan/vulkan_win32.h>

SurfaceManagerWin32::SurfaceManagerWin32() : m_surface{ VK_NULL_HANDLE }, m_pInstanceRef{ VK_NULL_HANDLE } {}

SurfaceManagerWin32::~SurfaceManagerWin32() noexcept {
	vkDestroySurfaceKHR(m_pInstanceRef, m_surface, nullptr);
}

void SurfaceManagerWin32::CreateSurface(VkInstance instance, void* windowHandle, void* moduleHandle)
{
	m_pInstanceRef = instance;

	VkWin32SurfaceCreateInfoKHR createInfo{
		.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = static_cast<HINSTANCE>(moduleHandle),
		.hwnd      = static_cast<HWND>(windowHandle)
	};

	vkCreateWin32SurfaceKHR(m_pInstanceRef, &createInfo, nullptr, &m_surface);
}

VkSurfaceKHR SurfaceManagerWin32::GetSurface() const noexcept {
	return m_surface;
}
