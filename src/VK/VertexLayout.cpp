#include <VertexLayout.hpp>

VertexLayout::VertexLayout() noexcept : m_createInfo{}, m_bindingDesc{} {
	InitLayout();
}

const VkPipelineVertexInputStateCreateInfo* VertexLayout::GetInputInfo() const noexcept {
	return &m_createInfo;
}

void VertexLayout::InitLayout() noexcept {
	std::uint32_t binding = 0u;

	// Position
	std::uint32_t vertexSize = 12u;
	m_attrDescs.emplace_back(0u, binding, VK_FORMAT_R32G32B32_SFLOAT, vertexSize);

	// UV
	vertexSize += 8u;
	m_attrDescs.emplace_back(1u, binding, VK_FORMAT_R32G32_SFLOAT, vertexSize);

	m_bindingDesc.binding = binding;
	m_bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_bindingDesc.stride = vertexSize;

	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_createInfo.vertexBindingDescriptionCount = 1u;
	m_createInfo.pVertexBindingDescriptions = &m_bindingDesc;
	m_createInfo.vertexAttributeDescriptionCount =
		static_cast<std::uint32_t>(std::size(m_attrDescs));
	m_createInfo.pVertexAttributeDescriptions = std::data(m_attrDescs);
}
