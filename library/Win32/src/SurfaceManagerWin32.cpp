#include <SurfaceManagerWin32.hpp>
#include <CleanWin.hpp>
#include <vulkan/vulkan_win32.h>

void SurfaceInstanceExtensionWin32::SetInstanceExtensions(
	VkInstanceExtensionManager& extensionManager
) noexcept {
	extensionManager.AddExtension(InstanceExtension::VkKhrSurface);
	extensionManager.AddExtension(InstanceExtension::VkKhrWin32Surface);
}

// Surface Manager Win32
void SurfaceManagerWin32::Create(VkInstance instance, void* windowHandle, void* moduleHandle)
{
	m_instance = instance;

	VkWin32SurfaceCreateInfoKHR createInfo{
		.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = static_cast<HINSTANCE>(moduleHandle),
		.hwnd      = static_cast<HWND>(windowHandle)
	};

	vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface);
}
