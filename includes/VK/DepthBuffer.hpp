#ifndef DEPTH_BUFFER_HPP_
#define DEPTH_BUFFER_HPP_
#include <vulkan/vulkan.hpp>

class DepthBuffer {
public:
	DepthBuffer(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		std::vector<std::uint32_t> queueFamilyIndices
	);
	~DepthBuffer() noexcept;

	void CleanUp(VkDevice device) noexcept;

	void CreateDepthBuffer(
		VkDevice device,
		std::uint32_t width, std::uint32_t height
	);

	[[nodiscard]]
	VkFormat GetDepthFormat() const noexcept;
	[[nodiscard]]
	VkImageView GetDepthImageView() const noexcept;

private:
	void CreateDepthImage(
		VkDevice device, VkImage* image,
		std::uint32_t width, std::uint32_t height,
		VkFormat depthFormat
	) const;
	void AllocateMemory(VkDevice device);

private:
	VkImage m_depthImage;
	VkImageView m_depthImageView;
	VkDeviceMemory m_depthMemory;

	VkDevice m_deviceRef;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	size_t m_memoryTypeIndex;

	static constexpr VkFormat DEPTHFORMAT = VK_FORMAT_D32_SFLOAT;
};
#endif
