#ifndef VK_BUFFERS_HPP_
#define VK_BUFFERS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class ImageBuffer {
public:
	ImageBuffer(VkDevice device) noexcept;
	~ImageBuffer() noexcept;

	void CreateImage(
		VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void CopyToImage(
		VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer,
		std::uint32_t width, std::uint32_t height
	) noexcept;
	void BindImageToMemory(
		VkDevice device, VkDeviceMemory memory, VkDeviceSize offset
	);
	void CreateImageView(VkDevice device, VkFormat format) noexcept;
	void TransitionImageLayout(VkCommandBuffer cmdBuffer, bool shaderStage) noexcept;

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
