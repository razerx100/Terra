#ifndef __I_RENDER_PASS_MANAGER_HPP__
#define __I_RENDER_PASS_MANAGER_HPP__
#include <vulkan/vulkan.hpp>

class IRenderPassManager {
public:
	virtual ~IRenderPassManager() = default;

	[[nodiscard]]
	virtual VkRenderPass GetRenderPass() const noexcept = 0;
	virtual void CreateRenderPass(VkDevice device, VkFormat swapchainFormat) = 0;
};

IRenderPassManager* CreateRenderPassManagerInstance(
	VkDevice device, VkFormat swapchainFormat
);
#endif
