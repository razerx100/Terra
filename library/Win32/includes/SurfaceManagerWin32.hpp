#ifndef SURFACE_MANAGER_WIN32_HPP_
#define SURFACE_MANAGER_WIN32_HPP_
#include <ISurfaceManager.hpp>

class SurfaceManagerWin32 final : public ISurfaceManager
{
public:
	SurfaceManagerWin32();
	~SurfaceManagerWin32() noexcept override;

	void CreateSurface(VkInstance instance, void* windowHandle, void* moduleHandle) override;

	[[nodiscard]]
	VkSurfaceKHR GetSurface() const noexcept override;

private:
	VkSurfaceKHR m_surface;
	VkInstance   m_pInstanceRef;

	inline static const std::vector<InstanceExtension> s_requiredExtensions
	{
		InstanceExtension::VkKhrWin32Surface
	};

public:
	[[nodiscard]]
	const std::vector<InstanceExtension>& GetRequiredExtensions() const noexcept override
	{ return s_requiredExtensions; }
};
#endif
