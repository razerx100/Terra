#ifndef __I_UPLOAD_BUFFERS_HPP__
#define __I_UPLOAD_BUFFERS_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

struct UploadBufferData {
	VkBuffer buffer;
	size_t bufferSize;
	size_t offset;
};

class IUploadBuffers {
public:
	virtual ~IUploadBuffers() = default;

	virtual void AddBuffer(VkDevice device, const void* data, size_t bufferSize) = 0;
	virtual void CopyData() noexcept = 0;
	virtual void CreateBuffers(VkDevice device) = 0;
	virtual void FlushMemory(VkDevice device) = 0;

	[[nodiscard]]
	virtual size_t GetMemoryAlignment() const noexcept = 0;
	[[nodiscard]]
	virtual const std::vector<UploadBufferData>& GetUploadBufferData() const noexcept = 0;
};
#endif
