#ifndef RESOURCE_BUFFER_HPP_
#define RESOURCE_BUFFER_HPP_
#include <vulkan/vulkan.hpp>
#include <DeviceMemory.hpp>
#include <memory>
#include <vector>
#include <UploadBuffers.hpp>
#include <VkBuffers.hpp>

class ResourceBuffer {
public:
	ResourceBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		std::vector<std::uint32_t> queueFamilyIndices,
		BufferType type
	);

	std::shared_ptr<GpuBuffer> AddBuffer(
		VkDevice device, std::unique_ptr<std::uint8_t> sourceHandle, size_t bufferSize
	);
	void CreateBuffers(VkDevice device);
	void CopyData() noexcept;
	void RecordUpload(VkDevice device, VkCommandBuffer copyCmdBuffer);
	void ReleaseUploadBuffer() noexcept;

private:
	struct GpuBufferData {
		size_t bufferSize;
		size_t offset;
	};

private:
	std::unique_ptr<UploadBuffers> m_uploadBuffers;
	std::unique_ptr<DeviceMemory> m_gpuBufferMemory;
	std::vector<std::shared_ptr<GpuBuffer>> m_gpuBuffers;
	size_t m_currentOffset;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	std::vector<GpuBufferData> m_gpuBufferData;
	BufferType m_type;
};
#endif
