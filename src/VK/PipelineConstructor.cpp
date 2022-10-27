#include <PipelineConstructor.hpp>
#include <Shader.hpp>

std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}

std::unique_ptr<VkPipelineObject> CreateGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath
) noexcept {
	auto vs = std::make_unique<Shader>(device);
	vs->CreateShader(device, shaderPath + L"VertexShader.spv");

	auto fs = std::make_unique<Shader>(device);
	fs->CreateShader(device, shaderPath + L"FragmentShader.spv");

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateGraphicsPipeline(
		device, graphicsLayout, renderPass,
		VertexLayout()
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u)
		.InitLayout(),
		vs->GetByteCode(), fs->GetByteCode()
	);

	return pso;
}

std::unique_ptr<PipelineLayout> CreateComputePipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages
	// Doesn't do anything different now but may in the future idk

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}

std::unique_ptr<VkPipelineObject> CreateComputePipeline(
	VkDevice device, VkPipelineLayout computeLayout, const std::wstring& shaderPath
) noexcept {
	auto cs = std::make_unique<Shader>(device);
	cs->CreateShader(device, shaderPath + L"ComputeShader.spv");

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateComputePipeline(device, computeLayout, cs->GetByteCode());

	return pso;
}
