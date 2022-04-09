#include <VertexLayout.hpp>

static const std::vector<VkFormat> vertexElementTypeFormatMap{
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_SFLOAT
};

static const std::vector<size_t> vertexElementTypeSizeMap{
	12u,
	16u
};

VertexLayout::VertexLayout() noexcept : m_createInfo{}, m_bindingDesc{} {}

VertexLayout::VertexLayout(const std::vector<VertexElementType>& inputLayout) noexcept
	: m_createInfo{}, m_bindingDesc {} {
	InitLayout(inputLayout);
}

const VkPipelineVertexInputStateCreateInfo* VertexLayout::GetInputInfo() const noexcept {
	return &m_createInfo;
}

void VertexLayout::InitLayout(const std::vector<VertexElementType>& inputLayout) noexcept {
	size_t vertexSize = 0u;
	for (size_t index = 0u; index < inputLayout.size(); ++index) {
		const auto elementTypeID = static_cast<size_t>(inputLayout[index]);

		m_attrDescs.emplace_back(
			static_cast<std::uint32_t>(index),
			0u,
			vertexElementTypeFormatMap[elementTypeID],
			static_cast<std::uint32_t>(vertexSize)
		);

		vertexSize += vertexElementTypeSizeMap[elementTypeID];
	}

	m_bindingDesc.binding = 0u;
	m_bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	m_bindingDesc.stride = static_cast<std::uint32_t>(vertexSize);

	m_createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	m_createInfo.vertexBindingDescriptionCount = 1u;
	m_createInfo.pVertexBindingDescriptions = &m_bindingDesc;
	m_createInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(m_attrDescs.size());
	m_createInfo.pVertexAttributeDescriptions = m_attrDescs.data();
}
