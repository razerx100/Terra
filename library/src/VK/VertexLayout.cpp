#include <VertexLayout.hpp>

VertexLayout::VertexLayout() noexcept
	: m_createInfo{}, m_bindingDesc{}, m_binding{ 0u }, m_vertexOffset{ 0u }, m_position{ 0u } {}

const VkPipelineVertexInputStateCreateInfo* VertexLayout::GetInputInfo() const noexcept {
	return &m_createInfo;
}

VertexLayout& VertexLayout::AddInput(VkFormat format, std::uint32_t sizeInBytes) noexcept {
	m_attrDescs.emplace_back(m_position, m_binding, format, m_vertexOffset);

	++m_position;
	m_vertexOffset += sizeInBytes;

	return *this;
}

VertexLayout& VertexLayout::InitLayout() noexcept {
	// These are member data, since the pointer of create info is required in the
	// GetInputInfo method
	m_bindingDesc.binding = m_binding;
	m_bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_bindingDesc.stride = m_vertexOffset;

	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_createInfo.vertexBindingDescriptionCount = 1u;
	m_createInfo.pVertexBindingDescriptions = &m_bindingDesc;
	m_createInfo.vertexAttributeDescriptionCount =
		static_cast<std::uint32_t>(std::size(m_attrDescs));
	m_createInfo.pVertexAttributeDescriptions = std::data(m_attrDescs);

	return *this;
}
