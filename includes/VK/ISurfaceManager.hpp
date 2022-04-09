#ifndef __I_SURFACE_MANAGER_HPP__
#define __I_SURFACE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class ISurfaceManager {
public:
	virtual ~ISurfaceManager() = default;

	[[nodiscard]]
	virtual VkSurfaceKHR GetSurface() const noexcept = 0;
};

#ifdef TERRA_WIN32
ISurfaceManager* CreateWin32SurfaceManagerInstance(
	VkInstance instance, void* windowHandle, void* moduleHandle
);
#endif
#endif
