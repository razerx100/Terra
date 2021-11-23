#ifndef __I_DISPLAY_MANAGER_HPP__
#define __I_DISPLAY_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>
#include <SUtility.hpp>

class IDisplayManager {
public:
	virtual ~IDisplayManager() = default;

	virtual const std::vector<const char*>& GetRequiredExtensions() const noexcept = 0;
	virtual void GetDisplayResolution(VkPhysicalDevice gpu, SRect& displayRect) = 0;
};

IDisplayManager* GetDisplayManagerInstance() noexcept;
void InitDisplayManagerInstance(VkPhysicalDevice gpu);
void CleanUpDisplayManagerInstance() noexcept;

#endif
