#ifndef __UPLOAD_BUFFERS_HPP__
#define __UPLOAD_BUFFERS_HPP__
#include <IUploadBuffers.hpp>
#include <IDeviceMemory.hpp>
#include <memory>

class UploadBuffers : public IUploadBuffers {
public:
	UploadBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);
	~UploadBuffers() noexcept override;

	void AddBuffer(VkDevice device, const void* data, size_t bufferSize) override;

	void CopyData() noexcept override;
	void CreateBuffers(VkDevice device) override;
	void FlushMemory(VkDevice device) override;

	[[nodiscard]]
	size_t GetMemoryAlignment() const noexcept override;
	[[nodiscard]]
	const std::vector<UploadBufferData>& GetUploadBufferData() const noexcept override;

private:
	VkDevice m_deviceRef;
	std::unique_ptr<IDeviceMemory> m_uploadMemory;
	std::vector<UploadBufferData> m_uploadBufferData;
	std::vector<const void*> m_dataHandles;
	VkBufferCreateInfo m_createInfo;
	std::uint8_t* m_cpuHandle;
	size_t m_currentOffset;
};
#endif
