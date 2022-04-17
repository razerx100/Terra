#ifndef I_DISPLAY_MANAGER_HPP_
#define I_DISPLAY_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <CRSStructures.hpp>

class IDisplayManager {
public:
	virtual ~IDisplayManager() = default;

	virtual void InitDisplayManager(VkPhysicalDevice gpu) = 0;
	virtual const std::vector<const char*>& GetRequiredExtensions() const noexcept = 0;
	virtual void GetDisplayResolution(VkPhysicalDevice gpu, Ceres::Rect& displayRect) = 0;
};
#endif
