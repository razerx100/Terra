#ifndef __I_VK_INSTANCE_MANAGER_HPP__
#define __I_VK_INSTANCE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class IInstanceManager {
public:
	virtual ~IInstanceManager() = default;

	virtual VkInstance GetVKInstance() const noexcept = 0;
};

IInstanceManager* CreateInstanceManagerInstance(const char* appName);

#endif
