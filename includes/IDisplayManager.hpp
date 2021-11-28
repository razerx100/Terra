#ifndef __I_DISPLAY_MANAGER_HPP__
#define __I_DISPLAY_MANAGER_HPP__
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

IDisplayManager* GetDisplayManagerInstance() noexcept;
void InitDisplayManagerInstance();
void CleanUpDisplayManagerInstance() noexcept;

#endif
