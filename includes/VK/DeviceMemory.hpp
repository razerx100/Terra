#ifndef DEVICE_MEMORY_HPP_
#define DEVICE_MEMORY_HPP_
#include <vulkan/vulkan.hpp>
#include <unordered_map>

class DeviceMemory {
public:
	DeviceMemory(VkDevice logicalDevice, std::uint32_t memoryTypeIndex) noexcept;
	DeviceMemory(DeviceMemory&& deviceMemory) noexcept;
	~DeviceMemory() noexcept;

	DeviceMemory& operator=(DeviceMemory&& deviceMemory) noexcept;

	DeviceMemory(const DeviceMemory&) = delete;
	DeviceMemory& operator=(const DeviceMemory&) = delete;

	void AllocateMemory(VkDevice device);

	[[nodiscard]]
	VkDeviceSize ReserveSizeAndGetOffset(
		VkDeviceSize memorySize, VkDeviceSize alignment
	) noexcept;
	[[nodiscard]]
	VkDeviceMemory GetMemoryHandle() const noexcept;

private:
	[[nodiscard]]
	static std::uint32_t FindMemoryTypeIndex(
		VkPhysicalDevice physicalDevice,
		const VkMemoryRequirements& memoryReq, VkMemoryPropertyFlags propertiesToCheck
	) noexcept;

	friend class DeviceMemoryManager;

private:
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	std::uint32_t m_memoryTypeIndex;
	VkDeviceSize m_totalSize;
};

class DeviceMemoryManager {
public:
	struct MemoryData{
		VkDeviceSize offset;
		std::uint32_t memoryTypeIndex;
	};

public:
	DeviceMemoryManager(VkPhysicalDevice physicalDevice) noexcept;

	void AllocateMemory(VkDevice device);

	[[nodiscard]]
	MemoryData ReserveSizeAndGetMemoryData(
		VkDevice device,
		const VkMemoryRequirements& memoryReq, VkMemoryPropertyFlags propertiesToCheck
	) noexcept;
	[[nodiscard]]
	VkDeviceMemory GetMemoryHandle(std::uint32_t memoryIndex) const noexcept;

private:
	VkPhysicalDevice m_physicalDeviceRef;
	std::unordered_map<std::uint32_t, DeviceMemory> m_memories;
};
#endif
