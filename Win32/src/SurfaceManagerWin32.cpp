#include <SurfaceManagerWin32.hpp>
#include <CleanWin.hpp>
#include <vulkan/vulkan_win32.h>

SurfaceManagerWin32::SurfaceManagerWin32(const Args& arguments)
	: m_surface{ VK_NULL_HANDLE }, m_pInstanceRef{ arguments.instance.value() } {

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = static_cast<HWND>(arguments.windowHandle.value());
	createInfo.hinstance = static_cast<HINSTANCE>(arguments.moduleHandle.value());

	vkCreateWin32SurfaceKHR(m_pInstanceRef, &createInfo, nullptr, &m_surface);
}

SurfaceManagerWin32::~SurfaceManagerWin32() noexcept {
	vkDestroySurfaceKHR(m_pInstanceRef, m_surface, nullptr);
}

VkSurfaceKHR SurfaceManagerWin32::GetSurface() const noexcept {
	return m_surface;
}
