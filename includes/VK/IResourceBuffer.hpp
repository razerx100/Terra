#ifndef __I_RESOURCE_BUFFER_HPP__
#define __I_RESOURCE_BUFFER_HPP__
#include <vulkan/vulkan.hpp>
#include <IDeviceMemory.hpp>

class IResourceBuffer {
public:
	virtual ~IResourceBuffer() = default;

	virtual VkBuffer AddBuffer(VkDevice device, const void* source, size_t bufferSize) = 0;
	virtual void CreateBuffer(VkDevice device) = 0;
	virtual void CopyData() noexcept = 0;
	virtual void RecordUpload(VkDevice device, VkCommandBuffer copyCmdBuffer) = 0;
	virtual void ReleaseUploadBuffer() noexcept = 0;
};

IResourceBuffer* CreateResourceBufferInstance(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices,
	BufferType type
);
#endif
