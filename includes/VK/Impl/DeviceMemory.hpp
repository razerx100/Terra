#ifndef __DEVICE_MEMORY_HPP__
#define __DEVICE_MEMORY_HPP__
#include <IDeviceMemory.hpp>
#include <vector>
#include <cstdint>

void ConfigureBufferQueueAccess(
	const std::vector<std::uint32_t>& queueFamilyIndices,
	VkBufferCreateInfo& bufferInfo
);

class DeviceMemory : public IDeviceMemory {
public:
	DeviceMemory(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices,
		bool uploadBuffer, BufferType type = BufferType::Invalid
	);
	~DeviceMemory() noexcept override;

	void AllocateMemory(
		size_t memorySize
	) override;

	[[nodiscard]]
	VkDeviceMemory GetMemoryHandle() const noexcept override;
	[[nodiscard]]
	size_t GetAlignment() const noexcept override;

private:
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	size_t m_memoryTypeIndex;
	size_t m_alignment;
};
#endif
