#ifndef __RENDER_PASS_MANAGER_HPP__
#define __RENDER_PASS_MANAGER_HPP__
#include <IRenderPassManager.hpp>

class RenderPassManager : public IRenderPassManager {
public:
	RenderPassManager(VkDevice device, VkFormat swapchainFormat);
	~RenderPassManager() noexcept;

	VkRenderPass GetRenderPass() const noexcept override;

	void CreateRenderPass(VkDevice device, VkFormat swapchainFormat) override;

private:
	VkDevice m_deviceRef;
	VkRenderPass m_renderPass;
};
#endif
