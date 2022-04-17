#ifndef RENDER_PASS_MANAGER_HPP_
#define RENDER_PASS_MANAGER_HPP_
#include <vulkan/vulkan.hpp>

class RenderPassManager {
public:
	RenderPassManager(VkDevice device, VkFormat swapchainFormat);
	~RenderPassManager() noexcept;

	[[nodiscard]]
	VkRenderPass GetRenderPass() const noexcept;

	void CreateRenderPass(VkDevice device, VkFormat swapchainFormat);

private:
	VkDevice m_deviceRef;
	VkRenderPass m_renderPass;
};
#endif
