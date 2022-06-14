#ifndef DEVICE_MEMORY_HPP_
#define DEVICE_MEMORY_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <cstdint>
#include <VkBuffers.hpp>

class DeviceMemory {
public:
	DeviceMemory(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		const VkMemoryRequirements& memoryRequirements, bool uploadBuffer
	);
	~DeviceMemory() noexcept;

	void AllocateMemory(size_t memorySize);

	[[nodiscard]]
	VkDeviceMemory GetMemoryHandle() const noexcept;

private:
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	size_t m_memoryTypeIndex;
};
#endif
