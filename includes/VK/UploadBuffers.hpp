#ifndef UPLOAD_BUFFERS_HPP_
#define UPLOAD_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DeviceMemory.hpp>
#include <VkBuffers.hpp>
#include <memory>

struct UploadBufferData {
	std::unique_ptr<UploadBuffer> buffer;
	size_t bufferSize;
	size_t offset;
};

class UploadBuffers {
public:
	UploadBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);

	void AddBuffer(VkDevice device, const void* data, size_t bufferSize);

	void CopyData() noexcept;
	void CreateBuffers(VkDevice device);
	void FlushMemory(VkDevice device);

	[[nodiscard]]
	size_t GetMemoryAlignment() const noexcept;
	[[nodiscard]]
	const std::vector<UploadBufferData>& GetUploadBufferData() const noexcept;

private:
	std::unique_ptr<DeviceMemory> m_uploadMemory;
	std::vector<UploadBufferData> m_uploadBufferData;
	std::vector<const void*> m_dataHandles;
	std::uint8_t* m_cpuHandle;
	size_t m_currentOffset;
};
#endif
