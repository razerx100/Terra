#include <SurfaceManagerWin32.hpp>
#include <CleanWin.hpp>
#include <vulkan/vulkan_win32.h>
#include <VKThrowMacros.hpp>

SurfaceManagerWin32::SurfaceManagerWin32(
	VkInstance instance, void* windowHandle, void* moduleHandle
) : m_pInstanceRef(instance) {

	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = reinterpret_cast<HWND>(windowHandle);
	createInfo.hinstance = reinterpret_cast<HINSTANCE>(moduleHandle);

	VkResult result;
	VK_THROW_FAILED(
		result,
		vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &m_surface)
	);
}

SurfaceManagerWin32::~SurfaceManagerWin32() noexcept {
	vkDestroySurfaceKHR(m_pInstanceRef, m_surface, nullptr);
}

VkSurfaceKHR SurfaceManagerWin32::GetSurface() const noexcept {
	return m_surface;
}
