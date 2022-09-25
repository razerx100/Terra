#ifndef VK_RESOURCE_VIEWS_HPP_
#define VK_RESOURCE_VIEWS_HPP_
#include <VkResources.hpp>
#include <cstdint>

enum class MemoryType {
	none,
	upload,
	cpuWrite,
	gpuOnly
};

class VkResourceView {
public:
	VkResourceView(VkDevice device) noexcept;

	VkResourceView(const VkResourceView&) = delete;
	VkResourceView& operator=(const VkResourceView&) = delete;

	VkResourceView(VkResourceView&& resourceView) noexcept;
	VkResourceView& operator=(VkResourceView&& resourceView) noexcept;

	void CreateResource(
		VkDevice device, VkDeviceSize bufferSize, std::uint32_t subAllocationCount,
		VkBufferUsageFlags usageFlags, std::vector<std::uint32_t> queueFamilyIndices = {}
	);
	void BindResourceToMemory(VkDevice device);
	void SetMemoryOffsetAndType(VkDevice device, MemoryType type) noexcept;
	void SetMemoryOffsetAndType(VkDeviceSize offset, MemoryType type) noexcept;
	void CleanUpResource() noexcept;
	void RecordCopy(VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer) noexcept;

	[[nodiscard]]
	VkBuffer GetResource() const noexcept;
	[[nodiscard]]
	VkDeviceSize GetMemoryOffset(VkDeviceSize index = 0u) const noexcept;
	[[nodiscard]]
	VkDeviceSize GetSubAllocationOffset(VkDeviceSize index) const noexcept;
	[[nodiscard]]
	VkDeviceSize GetSubAllocationSize() const noexcept;

private:
	[[nodiscard]]
	VkMemoryRequirements GetMemoryRequirements(VkDevice device) const noexcept;

private:
	VkResource m_resource;
	VkDeviceSize m_memoryOffsetStart;
	VkDeviceSize m_bufferSize;
	MemoryType m_resourceType;
	VkDeviceSize m_subAllocationSize;
};

class VkImageResourceView {
public:
	VkImageResourceView(VkDevice device) noexcept;
	~VkImageResourceView() noexcept;

	VkImageResourceView(const VkImageResourceView&) = delete;
	VkImageResourceView& operator=(const VkImageResourceView&) = delete;

	VkImageResourceView(VkImageResourceView&& imageResourceView) noexcept;
	VkImageResourceView& operator=(VkImageResourceView&& imageResourceView) noexcept;

	void CreateResource(
		VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void BindResourceToMemory(VkDevice device);
	void SetMemoryOffsetAndType(
		VkDevice device, MemoryType type = MemoryType::gpuOnly
	) noexcept;
	void SetMemoryOffsetAndType(
		VkDeviceSize offset, MemoryType type = MemoryType::gpuOnly
	) noexcept;

	void CreateImageView(VkDevice device, VkImageAspectFlagBits aspectBit);
	void RecordCopy(VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer) noexcept;
	void TransitionImageLayout(VkCommandBuffer cmdBuffer, bool shaderStage) noexcept;
	void CleanUpImageResourceView() noexcept;

	static void _createImageView(
		VkDevice device, VkImage image, VkImageView* imageView,
		VkFormat imageFormat, VkImageAspectFlagBits aspectBit
	);

	[[nodiscard]]
	VkImageView GetImageView() const noexcept;
	[[nodiscard]]
	VkImage GetResource() const noexcept;
	[[nodiscard]]
	VkDeviceSize GetMemoryOffset() const noexcept;

private:
	[[nodiscard]]
	VkMemoryRequirements GetMemoryRequirements(VkDevice device) const noexcept;

private:
	VkDevice m_deviceRef;
	VkImageResource m_resource;
	VkImageView m_imageView;
	std::uint32_t m_imageWidth;
	std::uint32_t m_imageHeight;
	VkFormat m_imageFormat;
	VkDeviceSize m_memoryOffset;
	MemoryType m_resourceType;
};

template<class ResourceView>
class VkUploadableResourceView {
public:
	VkUploadableResourceView(VkDevice device) noexcept
		: m_uploadResource{ device }, m_gpuResource{ device } {}

	VkUploadableResourceView(const VkUploadableResourceView&) = delete;
	VkUploadableResourceView& operator=(const VkUploadableResourceView&) = delete;

	VkUploadableResourceView(VkUploadableResourceView&& resourceView) noexcept
		: m_uploadResource{ std::move(resourceView.m_uploadResource) },
		m_gpuResource{ std::move(resourceView.m_gpuResource) } {}

	VkUploadableResourceView& operator=(VkUploadableResourceView&& resourceView) noexcept {
		m_uploadResource = std::move(resourceView.m_uploadResource);
		m_gpuResource = std::move(resourceView.m_gpuResource);

		return *this;
	}

	void BindResourceToMemory(VkDevice device	) {
		m_uploadResource.BindResourceToMemory(device);
		m_gpuResource.BindResourceToMemory(device);
	}

	void SetMemoryOffsetAndType(
		VkDevice device, MemoryType type = MemoryType::gpuOnly
	) noexcept {
		m_uploadResource.SetMemoryOffsetAndType(device, MemoryType::upload);
		m_gpuResource.SetMemoryOffsetAndType(device, type);
	}

	void CleanUpUploadResource() noexcept {
		m_uploadResource.CleanUpResource();
	}

	void RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept {
		m_gpuResource.RecordCopy(copyCmdBuffer, m_uploadResource.GetResource());
	}

	[[nodiscard]]
	auto GetGPUResource() const noexcept {
		return m_gpuResource.GetResource();
	}

	[[nodiscard]]
	VkDeviceSize GetUploadMemoryOffset(VkDeviceSize index = 0u) const noexcept {
		return m_uploadResource.GetMemoryOffset(index);
	}

protected:
	VkResourceView m_uploadResource;
	ResourceView m_gpuResource;
};

class VkUploadableBufferResourceView : public VkUploadableResourceView<VkResourceView> {
public:
	VkUploadableBufferResourceView(VkDevice device) noexcept;

	VkUploadableBufferResourceView(const VkUploadableBufferResourceView&) = delete;
	VkUploadableBufferResourceView& operator=(const VkUploadableBufferResourceView&) = delete;

	VkUploadableBufferResourceView(VkUploadableBufferResourceView&& resourceView) noexcept;
	VkUploadableBufferResourceView& operator=(
		VkUploadableBufferResourceView&& resourceView
		) noexcept;

	void CreateResource(
		VkDevice device, VkDeviceSize bufferSize, std::uint32_t subAllocationCount,
		VkBufferUsageFlags gpuBufferType, std::vector<std::uint32_t> queueFamilyIndices
	);

	[[nodiscard]]
	VkDeviceSize GetSubAllocationSize() const noexcept;
	[[nodiscard]]
	VkDeviceSize GetSubAllocationOffset(VkDeviceSize index) const noexcept;
};

class VkUploadableImageResourceView : public VkUploadableResourceView<VkImageResourceView> {
public:
	VkUploadableImageResourceView(VkDevice device) noexcept;

	VkUploadableImageResourceView(const VkUploadableImageResourceView&) = delete;
	VkUploadableImageResourceView& operator=(const VkUploadableImageResourceView&) = delete;

	VkUploadableImageResourceView(VkUploadableImageResourceView&& resourceView) noexcept;
	VkUploadableImageResourceView& operator=(
		VkUploadableImageResourceView&& resourceView
		) noexcept;

	void CreateResource(
		VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		std::vector<std::uint32_t> queueFamilyIndices
	);
	void TransitionImageLayout(VkCommandBuffer cmdBuffer) noexcept;
	void CreateImageView(VkDevice device, VkImageAspectFlagBits aspectBit);

	[[nodiscard]]
	VkImageView GetImageView() const noexcept;
};
#endif
