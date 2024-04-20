#include <VertexLayout.hpp>

VertexLayout& VertexLayout::AddInput(VkFormat format, std::uint32_t sizeInBytes) noexcept
{
	m_attrDescs.emplace_back(m_position, m_binding, format, m_vertexOffset);

	++m_position;
	m_vertexOffset += sizeInBytes;

	return *this;
}

VertexLayout& VertexLayout::InitLayout() noexcept
{
	m_bindingDesc.binding = m_binding;
	m_bindingDesc.stride  = m_vertexOffset;

	m_createInfo.vertexBindingDescriptionCount   = 1u;
	m_createInfo.pVertexBindingDescriptions      = &m_bindingDesc;
	m_createInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(std::size(m_attrDescs));
	m_createInfo.pVertexAttributeDescriptions    = std::data(m_attrDescs);

	return *this;
}

void VertexLayout::UpdatePointers() noexcept
{
	m_createInfo.pVertexBindingDescriptions   = &m_bindingDesc;
	m_createInfo.pVertexAttributeDescriptions = std::data(m_attrDescs);
}
