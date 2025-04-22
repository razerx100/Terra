#ifndef VK_SURFACE_MANAGER_WIN32_HPP_
#define VK_SURFACE_MANAGER_WIN32_HPP_
#include <VkSurfaceManager.hpp>

namespace SurfaceInstanceExtensionWin32
{
	void SetInstanceExtensions(VkInstanceExtensionManager& extensionManager) noexcept;
};

class SurfaceManagerWin32 : public SurfaceManager
{
public:
	SurfaceManagerWin32() : SurfaceManager{} {}

	void Create(VkInstance instance, void* windowHandle, void* moduleHandle);

public:
	SurfaceManagerWin32(const SurfaceManagerWin32&) = delete;
	SurfaceManagerWin32& operator=(const SurfaceManagerWin32&) = delete;

	SurfaceManagerWin32(SurfaceManagerWin32&& other) noexcept
		: SurfaceManager{ std::move(other) }
	{}
	SurfaceManagerWin32& operator=(SurfaceManagerWin32&& other) noexcept
	{
		SurfaceManager::operator=(std::move(other));

		return *this;
	}
};
#endif
