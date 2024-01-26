#ifndef VERTEX_LAYOUT_HPP_
#define VERTEX_LAYOUT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class VertexLayout {
public:
	VertexLayout() noexcept;

	[[nodiscard]]
	VertexLayout& AddInput(VkFormat format, std::uint32_t sizeInBytes) noexcept;
	[[nodiscard]]
	VertexLayout& InitLayout() noexcept;

	[[nodiscard]]
	const VkPipelineVertexInputStateCreateInfo* GetInputInfo() const noexcept;

private:
	VkPipelineVertexInputStateCreateInfo m_createInfo;
	VkVertexInputBindingDescription m_bindingDesc;
	std::vector<VkVertexInputAttributeDescription> m_attrDescs;

	std::uint32_t m_binding;
	std::uint32_t m_vertexOffset;
	std::uint32_t m_position;
};
#endif
