#ifndef VK_RENDER_PASS_HPP_
#define VK_RENDER_PASS_HPP_
#include <vulkan/vulkan.hpp>

class VKRenderPass {
public:
	VKRenderPass(VkDevice device) noexcept;
	~VKRenderPass() noexcept;

	[[nodiscard]]
	VkRenderPass GetRenderPass() const noexcept;

	void CreateRenderPass(VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat);

private:
	VkAttachmentDescription GetColourAttachment(VkFormat colourFormat) const noexcept;
	VkAttachmentDescription GetDepthAttachment(VkFormat depthFormat) const noexcept;

private:
	VkDevice m_deviceRef;
	VkRenderPass m_renderPass;
};
#endif
