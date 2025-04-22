#include <VkGraphicsPipelineBase.hpp>
#include <VkExternalFormatMap.hpp>

void ConfigurePipelineBuilder(
	GraphicsPipelineBuilder& builder, const ExternalGraphicsPipeline& graphicsExtPipeline
) noexcept {
	if (graphicsExtPipeline.GetBackfaceCullingState())
		builder.SetCullMode(VK_CULL_MODE_BACK_BIT);

	{
		VkFormat depthFormat   = GetVkFormat(graphicsExtPipeline.GetDepthFormat());
		VkFormat stencilFormat = GetVkFormat(graphicsExtPipeline.GetStencilFormat());

		const bool isDepthFormatValid   = depthFormat != VK_FORMAT_UNDEFINED;
		const bool isStencilFormatValid = stencilFormat != VK_FORMAT_UNDEFINED;

		const VkBool32 depthWrite = graphicsExtPipeline.IsDepthWriteEnabled() ? VK_TRUE : VK_FALSE;

		if (isDepthFormatValid || isStencilFormatValid)
			builder.SetDepthStencilState(
				DepthStencilStateBuilder{}.Enable(
					isDepthFormatValid ? VK_TRUE : VK_FALSE, depthWrite, VK_FALSE,
					isStencilFormatValid ? VK_TRUE : VK_FALSE
				), depthFormat, stencilFormat
			);
	}

	const size_t colourAttachmentCount = graphicsExtPipeline.GetRenderTargetCount();

	for (size_t index = 0u; index < colourAttachmentCount; ++index)
		builder.AddColourAttachment(
			GetVkFormat(graphicsExtPipeline.GetRenderTargetFormat(index)),
			GetVkBlendState(graphicsExtPipeline.GetBlendState(index))
		);
}
