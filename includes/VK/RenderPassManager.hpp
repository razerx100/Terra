#ifndef RENDER_PASS_MANAGER_HPP_
#define RENDER_PASS_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <optional>

class RenderPassManager {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<VkFormat> swapchainFormat;
		std::optional<VkFormat> depthFormat;
	};

public:
	RenderPassManager(const Args& arguments);
	~RenderPassManager() noexcept;

	[[nodiscard]]
	VkRenderPass GetRenderPass() const noexcept;

	void CreateRenderPass(
		VkDevice device, VkFormat swapchainFormat, VkFormat depthFormat
	);

private:
	VkAttachmentDescription GetColourAttachment(VkFormat colourFormat) const noexcept;
	VkAttachmentDescription GetDepthAttachment(VkFormat depthFormat) const noexcept;

private:
	VkDevice m_deviceRef;
	VkRenderPass m_renderPass;
};
#endif
