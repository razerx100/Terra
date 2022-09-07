#ifndef UPLOAD_BUFFERS_HPP_
#define UPLOAD_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DeviceMemory.hpp>
#include <memory>
#include <VkResourceViews.hpp>

class _CpuBaseBuffers {
public:
	virtual ~_CpuBaseBuffers() = default;

protected:
	struct BufferData {
		size_t bufferSize;
		VkDeviceSize offset;
	};

protected:
	void _bindMemories(VkDevice device, VkDeviceMemory memoryStart);

protected:
	std::vector<BufferData> m_allocationData;
	std::vector<std::shared_ptr<VkResourceView>> m_pBuffers;
};

class UploadBuffers : public _CpuBaseBuffers {
public:
	void BindMemories(VkDevice device);

	void AddBuffer(
		VkDevice device, std::unique_ptr<std::uint8_t> dataHandles, size_t bufferSize
	);

	void CopyData() noexcept;

	[[nodiscard]]
	const std::vector<std::shared_ptr<VkResourceView>>& GetUploadBuffers() const noexcept;

private:
	std::vector<std::unique_ptr<std::uint8_t>> m_dataHandles;
};

class HostAccessibleBuffers : public _CpuBaseBuffers {
public:
	void BindMemories(VkDevice device);

	std::shared_ptr<VkResourceView> AddBuffer(VkDevice device, size_t bufferSize);

	void ResetBufferData() noexcept;
};
#endif
