#ifndef RESOURCE_BUFFER_HPP_
#define RESOURCE_BUFFER_HPP_
#include <vulkan/vulkan.hpp>
#include <DeviceMemory.hpp>
#include <memory>
#include <vector>
#include <UploadBuffers.hpp>

class ResourceBuffer {
public:
	ResourceBuffer(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices,
		BufferType type
	);

	VkBuffer AddBuffer(VkDevice device, const void* source, size_t bufferSize);
	void CreateBuffer(VkDevice device);
	void CopyData() noexcept;
	void RecordUpload(VkDevice device, VkCommandBuffer copyCmdBuffer);
	void ReleaseUploadBuffer() noexcept;

private:
	std::unique_ptr<UploadBuffers> m_uploadBuffers;
	std::unique_ptr<DeviceMemory> m_gpuBufferMemory;
	std::vector<VkBuffer> m_gpuBuffers;
	size_t m_currentOffset;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	VkBufferCreateInfo m_gpuBufferCreateInfo;
};
#endif
