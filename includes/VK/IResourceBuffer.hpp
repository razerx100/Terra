#ifndef __I_RESOURCE_BUFFER_HPP__
#define __I_RESOURCE_BUFFER_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

class IResourceBuffer {
public:
	virtual ~IResourceBuffer() = default;

	virtual void CreateBuffer(
		size_t bufferSize,
		size_t memoryTypeIndex
	) = 0;

	virtual VkDeviceMemory GetGPUHandle() const noexcept = 0;
	virtual std::uint8_t* GetCPUHandle() const noexcept = 0;

	virtual void MapCPU() noexcept = 0;
	virtual void UnMapCPU() noexcept = 0;
};

#endif
