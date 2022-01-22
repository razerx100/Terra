#ifndef __DEVICE_MEMORY_HPP__
#define __DEVICE_MEMORY_HPP__
#include <IDeviceMemory.hpp>

class DeviceMemory : public IDeviceMemory {
public:
	DeviceMemory(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		bool uploadBuffer, BufferType type = BufferType::Invalid
	);
	~DeviceMemory() noexcept;

	void AllocateMemory(
		size_t memorySize
	) override;

	VkDeviceMemory GetMemoryHandle() const noexcept override;
	size_t GetAlignment() const noexcept override;

private:
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	size_t m_memoryTypeIndex;
	size_t m_alignment;
};
#endif
