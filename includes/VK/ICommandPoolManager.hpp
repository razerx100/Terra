#ifndef __I_COMMAND_POOL_MANAGER_HPP__
#define __I_COMMAND_POOL_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

class ICommandPoolManager {
public:
	virtual ~ICommandPoolManager() = default;

	virtual void Reset(std::uint32_t allocIndex) = 0;
	virtual void Close() = 0;
	virtual VkCommandBuffer GetCommandBuffer() const noexcept = 0;
};

ICommandPoolManager* GetGraphicsPoolManagerInstance() noexcept;
void InitGraphicsPoolManagerInstance(
	VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount
);
void CleanUpGraphicsPoolManagerInstance() noexcept;
#endif
