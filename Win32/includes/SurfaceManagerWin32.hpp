#ifndef SURFACE_MANAGER_WIN32_HPP_
#define SURFACE_MANAGER_WIN32_HPP_
#include <ISurfaceManager.hpp>
#include <optional>

class SurfaceManagerWin32 final : public ISurfaceManager {
public:
	struct Args {
		std::optional<VkInstance> instance;
		std::optional<void*> windowHandle;
		std::optional<void*> moduleHandle;
	};

public:
	SurfaceManagerWin32(const Args& arguments);
	~SurfaceManagerWin32() noexcept override;

	[[nodiscard]]
	VkSurfaceKHR GetSurface() const noexcept override;

private:
	VkSurfaceKHR m_surface;
	VkInstance m_pInstanceRef;
};
#endif
