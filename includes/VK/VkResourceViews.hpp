#ifndef VK_RESOURCE_VIEWS_HPP_
#define VK_RESOURCE_VIEWS_HPP_
#include <VkResources.hpp>
#include <cstdint>

class VkResourceView {
public:
	VkResourceView(VkDevice device) noexcept;

	VkResourceView(const VkResourceView&) = delete;
	VkResourceView& operator=(const VkResourceView&) = delete;

	VkResourceView(VkResourceView&& resourceView) noexcept;
	VkResourceView& operator=(VkResourceView&& resourceView) noexcept;

	void CreateResource(
		VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
		std::vector<std::uint32_t> queueFamilyIndices = {}
	);
	void BindResourceToMemory(VkDevice device, VkDeviceMemory memory);
	void SetMemoryOffset(VkDeviceSize offset) noexcept;

	[[nodiscard]]
	VkBuffer GetResource() const noexcept;
	[[nodiscard]]
	VkDeviceSize GetMemoryOffset() const noexcept;

private:
	VkResource m_resource;
	VkDeviceSize m_memoryOffset;
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
	void BindResourceToMemory(VkDevice device, VkDeviceMemory memory);
	void SetMemoryOffset(VkDeviceSize offset) noexcept;

	void CreateImageView(VkDevice device, VkImageAspectFlagBits aspectBit);
	void RecordImageCopy(VkCommandBuffer copyCmdBuffer, VkBuffer uploadBuffer) noexcept;
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

private:
	VkDevice m_deviceRef;
	VkImageResource m_resource;
	VkImageView m_imageView;
	std::uint32_t m_imageWidth;
	std::uint32_t m_imageHeight;
	VkFormat m_imageFormat;
	VkDeviceSize m_memoryOffset;
};

//class VkUploadableResourceView {
//public:
//	VkUploadableResourceView(VkDevice device) noexcept
//		: m_uploadResource{ device }, m_gpuResource{ device } {}
//
//private:
//	VkResourceView m_uploadResource;
//	VkImageResourceView m_gpuResource;
//};
#endif
