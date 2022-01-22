#include <VertexLayout.hpp>

static const std::vector<VkFormat> vertexElementTypeFormatMap{
	VK_FORMAT_R32G32B32_SFLOAT,
	VK_FORMAT_R32G32B32A32_SFLOAT
};

static const std::vector<size_t> vertexElementTypeSizeMap{
	12u,
	16u
};

VertexLayout::VertexLayout(const std::vector<VertexElementType>& inputLayout)
	: m_bindingDesc{} {

	size_t vertexSize = 0u;
	for (size_t index = 0u; index < inputLayout.size(); ++index) {
		size_t elementTypeID = static_cast<size_t>(inputLayout[index]);

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
}

VkPipelineVertexInputStateCreateInfo VertexLayout::GetInputInfo() const noexcept {

	VkPipelineVertexInputStateCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	createInfo.vertexBindingDescriptionCount = 1u;
	createInfo.pVertexBindingDescriptions = &m_bindingDesc;
	createInfo.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(m_attrDescs.size());
	createInfo.pVertexAttributeDescriptions = m_attrDescs.data();

	return createInfo;
}
