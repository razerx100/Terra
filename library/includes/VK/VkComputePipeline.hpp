#ifndef VK_COMPUTE_PIPELINE_HPP_
#define VK_COMPUTE_PIPELINE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>
#include <VkCommandQueue.hpp>
#include <VkPipelineLayout.hpp>
#include <ExternalPipeline.hpp>

class ComputePipeline
{
public:
	ComputePipeline() : m_computePipeline{}, m_computeExternalPipeline{} {}

	void Create(
		VkDevice device, VkPipelineLayout computeLayout,
		const ExternalComputePipeline& computeExtPipeline, const std::wstring& shaderPath
	);
	void Create(
		VkDevice device, const PipelineLayout& computeLayout,
		const ExternalComputePipeline& computeExtPipeline, const std::wstring& shaderPath
	) {
		Create(device, computeLayout.Get(), computeExtPipeline, shaderPath);
	}
	void Recreate(VkDevice device, VkPipelineLayout computeLayout, const std::wstring& shaderPath);

	void Bind(const VKCommandBuffer& computeBuffer) const noexcept;

	[[nodiscard]]
	const ExternalComputePipeline& GetExternalPipeline() const noexcept
	{
		return m_computeExternalPipeline;
	}

private:
	[[nodiscard]]
	static std::unique_ptr<VkPipelineObject> _createComputePipeline(
		VkDevice device, VkPipelineLayout computeLayout,
		const ExternalComputePipeline& computeExtPipeline, const std::wstring& shaderPath
	);

private:
	std::unique_ptr<VkPipelineObject> m_computePipeline;
	ExternalComputePipeline           m_computeExternalPipeline;

	static constexpr ShaderBinaryType s_shaderBytecodeType = ShaderBinaryType::SPIRV;

public:
	ComputePipeline(const ComputePipeline&) = delete;
	ComputePipeline& operator=(const ComputePipeline&) = delete;

	ComputePipeline(ComputePipeline&& other) noexcept
		: m_computePipeline{ std::move(other.m_computePipeline) },
		m_computeExternalPipeline{ std::move(other.m_computeExternalPipeline) }
	{}
	ComputePipeline& operator=(ComputePipeline&& other) noexcept
	{
		m_computePipeline         = std::move(other.m_computePipeline);
		m_computeExternalPipeline = std::move(other.m_computeExternalPipeline);

		return *this;
	}
};
#endif
