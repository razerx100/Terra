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
	void BindResourceToMemory(
		VkDevice device, VkDeviceMemory memory, VkDeviceSize offset
	);
	void SetCPUWPtr(std::uint8_t* ptr) noexcept;

	[[nodiscard]]
	VkBuffer GetResource() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPtr() const noexcept;

private:
	VkResource m_resource;
	std::uint8_t* m_cpuWPtr;
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
	void BindResourceToMemory(
		VkDevice device, VkDeviceMemory memory, VkDeviceSize offset
	);

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
};
#endif
