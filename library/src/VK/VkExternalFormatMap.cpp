#include <array>
#include <VkExternalFormatMap.hpp>

static constexpr std::array s_externalFormatMap
{
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_R16G16B16A16_SFLOAT,
	VK_FORMAT_R8_UNORM,
	VK_FORMAT_R16_SFLOAT,
	VK_FORMAT_D32_SFLOAT,
	VK_FORMAT_D24_UNORM_S8_UINT,
	VK_FORMAT_S8_UINT
};

static constexpr std::array s_blendOPMap
{
	VK_BLEND_OP_ADD,
	VK_BLEND_OP_SUBTRACT,
	VK_BLEND_OP_REVERSE_SUBTRACT,
	VK_BLEND_OP_MIN,
	VK_BLEND_OP_MAX
};

static constexpr std::array s_blendFactorMap
{
	VK_BLEND_FACTOR_ONE,
	VK_BLEND_FACTOR_ZERO,
	VK_BLEND_FACTOR_SRC_COLOR,
	VK_BLEND_FACTOR_DST_COLOR,
	VK_BLEND_FACTOR_SRC_ALPHA,
	VK_BLEND_FACTOR_DST_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
	VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA
};

VkFormat GetVkFormat(ExternalFormat format) noexcept
{
	return s_externalFormatMap[static_cast<size_t>(format)];
}

ExternalFormat GetExternalFormat(VkFormat format) noexcept
{
	size_t formatIndex = 0u;

	for (size_t index = 0u; index < std::size(s_externalFormatMap); ++index)
		if (s_externalFormatMap[index] == format)
		{
			formatIndex = index;

			break;
		}

	return static_cast<ExternalFormat>(formatIndex);
}

VkBlendFactor GetVkBlendFactor(ExternalBlendFactor factor) noexcept
{
	return s_blendFactorMap[static_cast<size_t>(factor)];
}

VkBlendOp GetVkBlendOP(ExternalBlendOP op) noexcept
{
	return s_blendOPMap[static_cast<size_t>(op)];
}

VkPipelineColorBlendAttachmentState GetVkBlendState(const ExternalBlendState& blendState) noexcept
{
	return VkPipelineColorBlendAttachmentState
	{
		.blendEnable         = blendState.enabled ? VK_TRUE : VK_FALSE,
		.srcColorBlendFactor = GetVkBlendFactor(blendState.colourBlendSrc),
		.dstColorBlendFactor = GetVkBlendFactor(blendState.colourBlendDst),
		.colorBlendOp        = GetVkBlendOP(blendState.colourBlendOP),
		.srcAlphaBlendFactor = GetVkBlendFactor(blendState.alphaBlendSrc),
		.dstAlphaBlendFactor = GetVkBlendFactor(blendState.alphaBlendDst),
		.alphaBlendOp        = GetVkBlendOP(blendState.alphaBlendOP),
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
						| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};
}
