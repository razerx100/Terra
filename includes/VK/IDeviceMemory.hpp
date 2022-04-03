#ifndef __I_DEVICE_MEMORY_HPP__
#define __I_DEVICE_MEMORY_HPP__
#include <vulkan/vulkan.hpp>
#include <cstdint>

enum class BufferType {
	Vertex,
	Index,
	UniformAndStorage,
	Invalid
};

class IDeviceMemory {
public:
	virtual ~IDeviceMemory() = default;

	virtual void AllocateMemory(
		size_t memorySize
	) = 0;

	virtual VkDeviceMemory GetMemoryHandle() const noexcept = 0;
	virtual size_t GetAlignment() const noexcept = 0;
};

#endif
