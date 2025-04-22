#ifndef VK_DEVICE_MEMORY_HPP_
#define VK_DEVICE_MEMORY_HPP_
#include <vulkan/vulkan.hpp>
#include <utility>

class DeviceMemory
{
public:
	DeviceMemory(
		VkDevice device, VkDeviceSize size, std::uint32_t typeIndex, VkMemoryPropertyFlagBits type
	);
	~DeviceMemory() noexcept;

	[[nodiscard]]
	VkDeviceSize Size() const noexcept { return m_size; }
	[[nodiscard]]
	VkDeviceMemory Memory() const noexcept { return m_memory; }
	[[nodiscard]]
	std::uint8_t* CPUMemory() const noexcept { return m_mappedCPUMemory; }
	[[nodiscard]]
	std::uint32_t TypeIndex() const noexcept { return m_memoryTypeIndex; }
	[[nodiscard]]
	VkMemoryPropertyFlagBits Type() const noexcept { return m_memoryType; }

private:
	void Allocate(VkDeviceSize size);

	void SelfDestruct() noexcept;

private:
	VkDevice                 m_device;
	VkDeviceMemory           m_memory;
	VkDeviceSize             m_size;
	std::uint8_t*            m_mappedCPUMemory;
	std::uint32_t            m_memoryTypeIndex;
	VkMemoryPropertyFlagBits m_memoryType;

public:
	DeviceMemory(const DeviceMemory&) = delete;
	DeviceMemory& operator=(const DeviceMemory&) = delete;

	DeviceMemory(DeviceMemory&& other) noexcept
		: m_device{ other.m_device }, m_memory{ std::exchange(other.m_memory, VK_NULL_HANDLE) },
		m_size{ other.m_size }, m_mappedCPUMemory{ other.m_mappedCPUMemory },
		m_memoryTypeIndex{ other.m_memoryTypeIndex }, m_memoryType{ other.m_memoryType }
	{}

	DeviceMemory& operator=(DeviceMemory&& other) noexcept
	{
		// To destroy the previous object if there is one.
		SelfDestruct();

		m_device          = other.m_device;
		m_memory          = std::exchange(other.m_memory, VK_NULL_HANDLE);
		m_size            = other.m_size;
		m_mappedCPUMemory = other.m_mappedCPUMemory;
		m_memoryTypeIndex = other.m_memoryTypeIndex;
		m_memoryType      = other.m_memoryType;

		return *this;
	}
};
#endif
