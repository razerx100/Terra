#ifndef DEVICE_MEMORY_HPP_
#define DEVICE_MEMORY_HPP_
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <optional>

class DeviceMemory {
public:
	struct Args {
		std::optional<VkDevice> logicalDevice;
		std::optional<VkPhysicalDevice> physicalDevice;
		std::optional<VkMemoryPropertyFlagBits> memoryType;
	};

public:
	DeviceMemory(const Args& arguments);
	DeviceMemory(DeviceMemory&& deviceMemory) noexcept;
	~DeviceMemory() noexcept;

	DeviceMemory& operator=(DeviceMemory&& deviceMemory) noexcept;

	DeviceMemory(const DeviceMemory&) = delete;
	DeviceMemory& operator=(const DeviceMemory&) = delete;

	void AllocateMemory(VkDevice device);
	void MapMemoryToCPU(VkDevice device);

	[[nodiscard]]
	VkDeviceSize ReserveSizeAndGetOffset(const VkMemoryRequirements& memoryReq) noexcept;
	[[nodiscard]]
	VkDeviceMemory GetMemoryHandle() const noexcept;
	[[nodiscard]]
	bool CheckMemoryType(const VkMemoryRequirements& memoryReq) const noexcept;
	[[nodiscard]]
	std::uint8_t* GetMappedCPUPtr() const noexcept;

private:
	[[nodiscard]]
	static std::uint32_t FindMemoryTypeIndex(
		VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags propertiesToCheck
	) noexcept;

private:
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	std::uint32_t m_memoryTypeIndex;
	VkDeviceSize m_totalSize;
	VkMemoryPropertyFlagBits m_memoryType;
	std::uint8_t* m_mappedCPUPtr;
};
#endif
