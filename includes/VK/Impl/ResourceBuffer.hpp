#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <memory>
#include <vector>
#include <UploadBuffers.hpp>

class ResourceBuffer : public IResourceBuffer {
public:
	ResourceBuffer(
		VkDevice logDevice, VkPhysicalDevice phyDevice,
		const std::vector<std::uint32_t>& queueFamilyIndices,
		BufferType type
	);

	VkBuffer AddBuffer(VkDevice device, const void* source, size_t bufferSize) override;
	void CreateBuffer(VkDevice device) override;
	void CopyData() noexcept override;
	void RecordUpload(VkDevice device, VkCommandBuffer copyCmdBuffer) override;
	void ReleaseUploadBuffer() noexcept override;

private:
	std::unique_ptr<IUploadBuffers> m_uploadBuffers;
	std::unique_ptr<IDeviceMemory> m_gpuBufferMemory;
	std::vector<VkBuffer> m_gpuBuffers;
	size_t m_currentOffset;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	VkBufferCreateInfo m_gpuBufferCreateInfo;
};
#endif
