#ifndef I_SURFACE_MANAGER_HPP_
#define I_SURFACE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkExtensionManager.hpp>

class ISurfaceManager {
public:
	virtual ~ISurfaceManager() = default;

	virtual void CreateSurface(VkInstance instance, void* windowHandle, void* moduleHandle) = 0;

	[[nodiscard]]
	virtual VkSurfaceKHR GetSurface() const noexcept = 0;
	[[nodiscard]]
	virtual const std::vector<InstanceExtension>& GetRequiredExtensions() const noexcept = 0;
};
#endif
