#ifndef UPLOAD_BUFFERS_HPP_
#define UPLOAD_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DeviceMemory.hpp>
#include <VkBuffers.hpp>
#include <memory>

class _CpuBaseBuffers {
public:
	_CpuBaseBuffers() noexcept;
	virtual ~_CpuBaseBuffers() = default;

	void CreateBuffers(VkDevice device);
	void FlushMemory(VkDevice device);

protected:
	struct BufferData {
		size_t bufferSize;
		VkDeviceSize offset;
		std::uint8_t* cpuWritePtr;
	};

protected:
	virtual void AfterCreationStuff();

protected:
	std::vector<BufferData> m_allocationData;
	std::vector<std::shared_ptr<UploadBuffer>> m_pBuffers;
	std::uint32_t m_memoryTypeIndex;
};

class UploadBuffers : public _CpuBaseBuffers {
public:
	void AddBuffer(
		VkDevice device, std::unique_ptr<std::uint8_t> dataHandles, size_t bufferSize
	);

	void CopyData() noexcept;

	[[nodiscard]]
	const std::vector<std::shared_ptr<UploadBuffer>>& GetUploadBuffers() const noexcept;

private:
	std::vector<std::unique_ptr<std::uint8_t>> m_dataHandles;
};

class HostAccessibleBuffers : public _CpuBaseBuffers {
public:
	std::shared_ptr<UploadBuffer> AddBuffer(VkDevice device, size_t bufferSize);

	void ResetBufferData() noexcept;

private:
	void AfterCreationStuff() final;
};
#endif
