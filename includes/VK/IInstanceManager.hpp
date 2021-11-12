#ifndef __I_INSTANCE_MANAGER_HPP__
#define __I_INSTANCE_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class IInstanceManager {
public:
	virtual ~IInstanceManager() = default;

	virtual VkInstance GetVKInstance() const noexcept = 0;
};

IInstanceManager* GetInstanceManagerInstance() noexcept;
void InitInstanceManagerInstance(const char* appName);
void CleanUpInstanceManagerInstance() noexcept;
#endif
