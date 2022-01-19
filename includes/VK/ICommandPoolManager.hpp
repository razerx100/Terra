#ifndef __I_COMMAND_POOL_MANAGER_HPP__
#define __I_COMMAND_POOL_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

class ICommandPoolManager {
public:
	virtual ~ICommandPoolManager() = default;

	virtual void Reset(size_t bufferIndex) = 0;
	virtual void Close(size_t bufferIndex) = 0;
	virtual VkCommandBuffer GetCommandBuffer(size_t bufferIndex) const noexcept = 0;
};

ICommandPoolManager* CreateCommandPoolInstance(
	VkDevice device, size_t queueIndex, size_t bufferCount
);
#endif
