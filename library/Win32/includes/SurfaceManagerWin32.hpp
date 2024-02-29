#ifndef SURFACE_MANAGER_WIN32_HPP_
#define SURFACE_MANAGER_WIN32_HPP_
#include <SurfaceManager.hpp>

class SurfaceManagerWin32 final : public SurfaceManager
{
public:
	SurfaceManagerWin32() : SurfaceManager{}
	{
		m_requiredExtensions.emplace_back(InstanceExtension::VkKhrWin32Surface);
	}

	void Create(VkInstance instance, void* windowHandle, void* moduleHandle) override;

public:
	SurfaceManagerWin32(const SurfaceManagerWin32&) = delete;
	SurfaceManagerWin32& operator=(const SurfaceManagerWin32&) = delete;

	SurfaceManagerWin32(SurfaceManagerWin32&& other) noexcept : SurfaceManager{ std::move(other) } {}
	SurfaceManagerWin32& operator=(SurfaceManagerWin32&& other) noexcept
	{
		SurfaceManager::operator=(std::move(other));

		return *this;
	}
};
#endif
