#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <optional>

class ResourceBuffer : public IResourceBuffer {
public:
	ResourceBuffer(VkDevice device) noexcept;
	~ResourceBuffer() noexcept;

	void CreateBuffer(
		size_t bufferSize,
		size_t memoryTypeIndex
	) override;

	VkDeviceMemory GetGPUHandle() const noexcept override;
	std::uint8_t* GetCPUHandle() const noexcept override;

	void MapCPU() noexcept override;
	void UnMapCPU() noexcept override;

private:
	VkDevice m_deviceRef;
	VkDeviceMemory m_bufferMemory;
	std::uint8_t* m_cpuHandle;
};
#endif
