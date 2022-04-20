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
		VkDevice device, size_t bufferSize
	);
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
		std::uint32_t pixelSizeInBytes,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);

	[[nodiscard]]
	VkImage GetImage() const noexcept;

private:
	static  void ConfigureImageQueueAccess(
		const std::vector<std::uint32_t>& queueFamilyIndices,
		VkImageCreateInfo& imageInfo
	) noexcept;

private:
	VkDevice m_deviceRef;
	VkImage m_image;
};
#endif
