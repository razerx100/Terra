#include <SurfaceManagerWin32.hpp>
#include <CleanWin.hpp>
#include <vulkan/vulkan_win32.h>

SurfaceManagerWin32::SurfaceManagerWin32(
	VkInstance instance, void* windowHandle, void* moduleHandle
) : m_surface(VK_NULL_HANDLE), m_pInstanceRef(instance) {

	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = static_cast<HWND>(windowHandle);
	createInfo.hinstance = static_cast<HINSTANCE>(moduleHandle);

	vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_surface);
}

SurfaceManagerWin32::~SurfaceManagerWin32() noexcept {
	vkDestroySurfaceKHR(m_pInstanceRef, m_surface, nullptr);
}

VkSurfaceKHR SurfaceManagerWin32::GetSurface() const noexcept {
	return m_surface;
}
