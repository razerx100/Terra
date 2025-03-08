#include <GraphicsPipelineBase.hpp>

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

		if (isDepthFormatValid || isStencilFormatValid)
			builder.SetDepthStencilState(
				DepthStencilStateBuilder{}.Enable(
					isDepthFormatValid, isDepthFormatValid, VK_FALSE, isStencilFormatValid
				), depthFormat, stencilFormat
			);
	}

	// Will add Colour blending later.
	const size_t colourAttachmentCount = graphicsExtPipeline.GetRenderTargetCount();

	for (size_t index = 0u; index < colourAttachmentCount; ++index)
		builder.AddColourAttachment(GetVkFormat(graphicsExtPipeline.GetRenderTargetFormat(index)));
}
