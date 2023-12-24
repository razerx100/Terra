#ifndef SURFACE_MANAGER_WIN32_HPP_
#define SURFACE_MANAGER_WIN32_HPP_
#include <ISurfaceManager.hpp>

class SurfaceManagerWin32 final : public ISurfaceManager
{
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
