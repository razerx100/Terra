#ifndef UPLOAD_BUFFERS_HPP_
#define UPLOAD_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DeviceMemory.hpp>
#include <VkBuffers.hpp>
#include <memory>

class UploadBuffers {
public:
	UploadBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);

	void AddBuffer(VkDevice device, const void* data, size_t bufferSize);

	void CopyData() noexcept;
	void CreateBuffers(VkDevice device);
	void FlushMemory(VkDevice device);

	[[nodiscard]]
	const std::vector<std::unique_ptr<UploadBuffer>>& GetUploadBuffers() const noexcept;

private:
	struct UploadBufferData {
		size_t bufferSize;
		size_t offset;
	};

private:
	std::unique_ptr<DeviceMemory> m_uploadMemory;
	std::vector<UploadBufferData> m_uploadBufferData;
	std::vector<std::unique_ptr<UploadBuffer>> m_buffers;
	std::vector<const void*> m_dataHandles;
	std::uint8_t* m_cpuHandle;
	size_t m_currentOffset;
};

class UploadBufferSingle {
public:
	UploadBufferSingle(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);

	void CreateBuffer(VkDevice device, size_t bufferSize);
	void FlushMemory(VkDevice device);
	void CopyData(const void* data, size_t bufferSize);

	[[nodiscard]]
	VkBuffer GetBuffer() const noexcept;

private:
	std::uint8_t* m_pCpuHandle;
	size_t m_bufferSize;
	std::unique_ptr<UploadBuffer> m_pBuffer;
	std::unique_ptr<DeviceMemory> m_pBufferMemory;
};
#endif
