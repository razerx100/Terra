#ifndef __SURFACE_MANAGER_WIN32_HPP__
#define __SURFACE_MANAGER_WIN32_HPP__
#include <ISurfaceManager.hpp>

class SurfaceManagerWin32 : public ISurfaceManager {
public:
	SurfaceManagerWin32(VkInstance instance, void* windowHandle, void* moduleHandle);
	~SurfaceManagerWin32() noexcept override;

	[[nodiscard]]
	VkSurfaceKHR GetSurface() const noexcept override;

private:
	VkSurfaceKHR m_surface;
	VkInstance m_pInstanceRef;
};
#endif
