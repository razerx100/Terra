#ifndef __RESOURCE_BUFFER_HPP__
#define __RESOURCE_BUFFER_HPP__
#include <IResourceBuffer.hpp>
#include <memory>
#include <vector>

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
	void ReleaseUploadBuffer(VkDevice device) noexcept override;

private:
	struct BufferData {
		VkBuffer buffer;
		const void* data;
		size_t size;
		size_t offset;
	};

private:
	std::unique_ptr<IDeviceMemory> m_uploadBufferMemory;
	std::unique_ptr<IDeviceMemory> m_gpuBufferMemory;
	std::vector<BufferData> m_uploadBufferData;
	std::vector<VkBuffer> m_gpuBuffers;
	size_t m_currentOffset;
	std::uint8_t* m_cpuHandle;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	VkBufferCreateInfo m_uploadBufferCreateInfo;
	VkBufferCreateInfo m_gpuBufferCreateInfo;
};
#endif
