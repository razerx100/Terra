#ifndef DEVICE_MEMORY_HPP_
#define DEVICE_MEMORY_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <cstdint>
#include <VkBuffers.hpp>

class DeviceMemory {
public:
	DeviceMemory(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices,
		bool uploadBuffer, BufferType type = BufferType::Invalid
	);
	~DeviceMemory() noexcept;

	void AllocateMemory(
		size_t memorySize
	);

	[[nodiscard]]
	VkDeviceMemory GetMemoryHandle() const noexcept;
	[[nodiscard]]
	size_t GetAlignment() const noexcept;

private:
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	size_t m_memoryTypeIndex;
	size_t m_alignment;
};
#endif
