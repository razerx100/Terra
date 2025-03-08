#include <ComputePipeline.hpp>
#include <VkShader.hpp>

void ComputePipeline::Create(
	VkDevice device, VkPipelineLayout computeLayout, const ExternalComputePipeline& computeExtPipeline,
	const std::wstring& shaderPath
) {
	m_computeExternalPipeline = computeExtPipeline;

	m_computePipeline = _createComputePipeline(
		device, computeLayout, m_computeExternalPipeline, shaderPath
	);
}

void ComputePipeline::Recreate(
	VkDevice device, VkPipelineLayout computeLayout, const std::wstring& shaderPath
) {
	m_computePipeline = _createComputePipeline(
		device, computeLayout, m_computeExternalPipeline, shaderPath
	);
}

std::unique_ptr<VkPipelineObject> ComputePipeline::_createComputePipeline(
	VkDevice device, VkPipelineLayout computeLayout, const ExternalComputePipeline& computeExtPipeline,
	const std::wstring& shaderPath
) {
	auto cs            = std::make_unique<VkShader>(device);
	const bool success = cs->Create(
		shaderPath + computeExtPipeline.GetComputeShader().GetNameWithExtension(s_shaderBytecodeType)
	);

	auto pso = std::make_unique<VkPipelineObject>(device);

	if (success)
		pso->CreateComputePipeline(ComputePipelineBuilder{ computeLayout }.SetComputeStage(cs->Get()));

	return pso;
}

void ComputePipeline::Bind(const VKCommandBuffer& computeBuffer) const noexcept
{
	VkCommandBuffer cmdBuffer = computeBuffer.Get();

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipeline->Get());
}
