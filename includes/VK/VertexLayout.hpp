#ifndef VERTEX_LAYOUT_HPP_
#define VERTEX_LAYOUT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class VertexLayout {
public:
	VertexLayout() noexcept;

	[[nodiscard]]
	const VkPipelineVertexInputStateCreateInfo* GetInputInfo() const noexcept;

private:
	void InitLayout() noexcept;

private:
	VkPipelineVertexInputStateCreateInfo m_createInfo;
	VkVertexInputBindingDescription m_bindingDesc;
	std::vector<VkVertexInputAttributeDescription> m_attrDescs;
};
#endif
