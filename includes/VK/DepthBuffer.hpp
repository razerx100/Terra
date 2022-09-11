#ifndef DEPTH_BUFFER_HPP_
#define DEPTH_BUFFER_HPP_
#include <vulkan/vulkan.hpp>
#include <VkResourceViews.hpp>

class DepthBuffer {
public:
	DepthBuffer(VkDevice logicalDevice, std::vector<std::uint32_t> queueFamilyIndices);

	DepthBuffer(const DepthBuffer&) = delete;
	DepthBuffer& operator=(const DepthBuffer&) = delete;

	DepthBuffer(DepthBuffer&& depthBuffer) noexcept;
	DepthBuffer& operator=(DepthBuffer&& depthBuffer) noexcept;

	void CleanUp() noexcept;

	void CreateDepthBuffer(
		VkDevice device, std::uint32_t width, std::uint32_t height
	);
	void AllocateForMaxResolution(
		VkDevice device, std::uint32_t width, std::uint32_t height
	);

	[[nodiscard]]
	VkFormat GetDepthFormat() const noexcept;
	[[nodiscard]]
	VkImageView GetDepthImageView() const noexcept;

private:
	VkImageResourceView m_depthImage;

	VkDevice m_deviceRef;
	std::vector<std::uint32_t> m_queueFamilyIndices;

	std::uint32_t m_maxWidth;
	std::uint32_t m_maxHeight;

	static constexpr VkFormat DEPTHFORMAT = VK_FORMAT_D32_SFLOAT;
};
#endif
