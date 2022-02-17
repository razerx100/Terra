#ifndef __VERTEX_LAYOUT_HPP__
#define __VERTEX_LAYOUT_HPP__
#include <vulkan/vulkan.hpp>
#include <IModel.hpp>
#include <vector>

class VertexLayout {
public:
	VertexLayout() noexcept;
	VertexLayout(const std::vector<VertexElementType>& inputLayout) noexcept;

	void InitLayout(const std::vector<VertexElementType>& inputLayout) noexcept;

	const VkPipelineVertexInputStateCreateInfo* GetInputInfo() const noexcept;

private:
	VkPipelineVertexInputStateCreateInfo m_createInfo;
	VkVertexInputBindingDescription m_bindingDesc;
	std::vector<VkVertexInputAttributeDescription> m_attrDescs;
};
#endif
