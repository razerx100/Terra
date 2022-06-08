#ifndef UPLOAD_BUFFERS_HPP_
#define UPLOAD_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <DeviceMemory.hpp>
#include <VkBuffers.hpp>
#include <memory>

class _CpuBaseBuffers {
public:
	_CpuBaseBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);
	virtual ~_CpuBaseBuffers() = default;

	void CreateBuffers(VkDevice device);
	void FlushMemory(VkDevice device);

protected:
	struct BufferData {
		size_t bufferSize;
		size_t offset;
	};

protected:
	virtual void AfterCreationStuff();

protected:
	std::unique_ptr<DeviceMemory> m_pBufferMemory;
	std::vector<BufferData> m_allocationData;
	std::vector<std::shared_ptr<UploadBuffer>> m_pBuffers;
	std::uint8_t* m_cpuHandle;
	size_t m_currentOffset;
};

class UploadBuffers : public _CpuBaseBuffers {
public:
	UploadBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);

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
	HostAccessibleBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);

	std::shared_ptr<UploadBuffer> AddBuffer(
		VkDevice device, size_t bufferSize,
		VkBufferUsageFlags bufferStageFlag
	);

	void ResetBufferData() noexcept;

private:
	void AfterCreationStuff() final;
};
#endif
