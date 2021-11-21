#ifndef __I_COMMAND_POOL_MANAGER_HPP__
#define __I_COMMAND_POOL_MANAGER_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

class ICommandPoolManager {
public:
	virtual ~ICommandPoolManager() = default;

	virtual void Reset(std::uint32_t bufferIndex) = 0;
	virtual void Close(std::uint32_t bufferIndex) = 0;
	virtual VkCommandBuffer GetCommandBuffer(std::uint32_t bufferIndex) const noexcept = 0;
};

ICommandPoolManager* GetGraphicsPoolManagerInstance() noexcept;
void InitGraphicsPoolManagerInstance(
	VkDevice device, std::uint32_t queueIndex, std::uint32_t bufferCount
);
void CleanUpGraphicsPoolManagerInstance() noexcept;
#endif
