#ifndef VULKAN_BUFFERS_HPP_
#define VULKAN_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

enum class BufferType {
	Vertex,
	Index,
	UniformAndStorage,
	Image,
	Invalid
};

class BaseBuffer {
public:
	BaseBuffer(VkDevice device) noexcept;
	~BaseBuffer() noexcept;

	[[nodiscard]]
	VkBuffer GetBuffer() const noexcept;

protected:
	static void ConfigureBufferQueueAccess(
		const std::vector<std::uint32_t>& queueFamilyIndices,
		VkBufferCreateInfo& bufferInfo
	) noexcept;

protected:
	VkBuffer m_buffer;

private:
	VkDevice m_deviceRef;
};

class GpuBuffer final : public BaseBuffer {
public:
	GpuBuffer(
		VkDevice device
	) noexcept;

	void CreateBuffer(
		VkDevice device,
		size_t bufferSize,
		const std::vector<std::uint32_t>& queueFamilyIndices,
		BufferType type
	);
};

class UploadBuffer final : public BaseBuffer{
public:
	UploadBuffer(
		VkDevice device
	) noexcept;

	void CreateBuffer(
		VkDevice device, size_t bufferSize,
		VkBufferUsageFlags bufferFlags = 0u
	);

	void SetCpuHandle(std::uint8_t* cpuHandle) noexcept;

	[[nodiscard]]
	std::uint8_t* GetCpuHandle() const noexcept;

private:
	std::uint8_t* m_pCpuHandle;
};

class ImageBuffer {
public:
	ImageBuffer(
		VkDevice device
	) noexcept;
	~ImageBuffer() noexcept;

	void CreateImage(
		VkDevice device,
		std::uint32_t width, std::uint32_t height,
		VkFormat imageFormat,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void CopyToImage(
		VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer,
		std::uint32_t width, std::uint32_t height
	) noexcept;
	void BindImageToMemory(
		VkDevice device, VkDeviceMemory memory,
		VkDeviceSize offset
	);
	void CreateImageView(
		VkDevice device, VkFormat format
	) noexcept;
	void TransitionImageLayout(
		VkCommandBuffer cmdBuffer, bool shaderStage
	) noexcept;

	[[nodiscard]]
	VkImageView GetImageView() const noexcept;
	[[nodiscard]]
	VkImage GetImage() const noexcept;

private:
	VkDevice m_deviceRef;
	VkImage m_image;
	VkImageView m_imageView;
};
#endif
