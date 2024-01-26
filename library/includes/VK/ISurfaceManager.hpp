#ifndef I_SURFACE_MANAGER_HPP_
#define I_SURFACE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>

class ISurfaceManager {
public:
	virtual ~ISurfaceManager() = default;

	[[nodiscard]]
	virtual VkSurfaceKHR GetSurface() const noexcept = 0;
};
#endif
