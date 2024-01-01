#ifndef DEVICE_MEMORY_HPP_
#define DEVICE_MEMORY_HPP_
#include <vulkan/vulkan.hpp>
#include <unordered_map>

class DeviceMemory
{
public:
	DeviceMemory(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkMemoryPropertyFlagBits memoryType
	);
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
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	std::uint32_t m_memoryTypeIndex;
	VkDeviceSize m_totalSize;
	VkMemoryPropertyFlagBits m_memoryType;
	std::uint8_t* m_mappedCPUPtr;
};

class DeviceMemory2
{
public:
	DeviceMemory2(VkDevice device, VkDeviceSize size, std::uint32_t typeIndex, VkMemoryType type);
	~DeviceMemory2() noexcept;

	[[nodiscard]]
	inline VkDeviceSize Size() const noexcept { return m_size; }
	[[nodiscard]]
	inline VkDeviceMemory Memory() const noexcept { return m_memory; }
	[[nodiscard]]
	inline std::uint8_t* CPUMemory() const noexcept { return m_mappedCPUMemory; }
	[[nodiscard]]
	inline std::uint32_t TypeIndex() const noexcept { return m_memoryTypeIndex; }
	[[nodiscard]]
	inline VkMemoryType Type() const noexcept { return m_memoryType; }

private:
	void Allocate(VkDeviceSize size);

private:
	VkDevice                 m_device;
	VkDeviceMemory           m_memory;
	VkDeviceSize             m_size;
	std::uint8_t*            m_mappedCPUMemory;
	std::uint32_t            m_memoryTypeIndex;
	VkMemoryType             m_memoryType;

public:
	DeviceMemory2(const DeviceMemory2&) = delete;
	DeviceMemory2& operator=(const DeviceMemory2&) = delete;

	inline DeviceMemory2(DeviceMemory2&& other) noexcept
		: m_device{ other.m_device }, m_memory{ other.m_memory }, m_size{ other.m_size },
		m_mappedCPUMemory{ other.m_mappedCPUMemory }, m_memoryTypeIndex{ other.m_memoryTypeIndex },
		m_memoryType{ other.m_memoryType }
	{
		other.m_memory = VK_NULL_HANDLE; // Otherwise the dtor will destroy it.
	}

	inline DeviceMemory2& operator=(DeviceMemory2&& other) noexcept
	{
		m_device          = other.m_device;
		m_memory          = other.m_memory;
		m_size            = other.m_size;
		m_mappedCPUMemory = other.m_mappedCPUMemory;
		m_memoryTypeIndex = other.m_memoryTypeIndex;
		m_memoryType      = other.m_memoryType;

		other.m_memory    = VK_NULL_HANDLE; // Otherwise the dtor will destroy it.

		return *this;
	}
};
#endif
