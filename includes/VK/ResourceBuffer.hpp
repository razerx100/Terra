#ifndef RESOURCE_BUFFER_HPP_
#define RESOURCE_BUFFER_HPP_
#include <vulkan/vulkan.hpp>
#include <DeviceMemory.hpp>
#include <memory>
#include <vector>
#include <UploadBuffers.hpp>
#include <VkResourceViews.hpp>

class ResourceBuffer {
public:
	ResourceBuffer(std::vector<std::uint32_t> queueFamilyIndices, VkBufferUsageFlagBits type);

	std::shared_ptr<VkResourceView> AddBuffer(
		VkDevice device, std::unique_ptr<std::uint8_t> sourceHandle, size_t bufferSize
	);
	void BindMemories(VkDevice device);
	void CopyData() noexcept;
	void RecordUpload(VkCommandBuffer copyCmdBuffer);
	void ReleaseUploadBuffer() noexcept;

private:
	struct GpuBufferData {
		VkDeviceSize bufferSize;
		VkDeviceSize offset;
	};

private:
	std::unique_ptr<UploadBuffers> m_uploadBuffers;
	std::vector<std::shared_ptr<VkResourceView>> m_gpuBuffers;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	std::vector<GpuBufferData> m_gpuBufferData;
	VkBufferUsageFlagBits m_resourceType;
};
#endif
