#include <RenderEngineMeshShader.hpp>

RenderEngineMeshShader::RenderEngineMeshShader(VkDevice device) : RenderEngineBase{ device } {}

std::unique_ptr<PipelineLayout> RenderEngineMeshShader::CreateGraphicsPipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages
	static constexpr std::uint32_t pushConstantSize =
			static_cast<std::uint32_t>(sizeof(std::uint32_t) * 2u);

	pipelineLayout->AddPushConstantRange(VK_SHADER_STAGE_MESH_BIT_EXT, pushConstantSize);
	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}
