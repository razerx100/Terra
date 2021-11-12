#ifndef __I_SURFACE_MANAGER_HPP__
#define __I_SURFACE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class ISurfaceManager {
public:
	virtual ~ISurfaceManager() = default;

	virtual VkSurfaceKHR GetSurface() const noexcept = 0;
};

ISurfaceManager* GetSurfaceManagerInstance() noexcept;
void InitSurfaceManagerInstance(VkInstance instance, void* windowHandle, void* moduleHandle);
void CleanUpSurfaceManagerInstance() noexcept;
#endif
