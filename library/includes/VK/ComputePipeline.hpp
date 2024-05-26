#ifndef COMPUTE_PIPELINE_HPP_
#define COMPUTE_PIPELINE_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <string>
#include <VKPipelineObject.hpp>
#include <VkCommandQueue.hpp>
#include <PipelineLayout.hpp>

class ComputePipeline
{
public:
	ComputePipeline() : m_computePipeline{} {}

	void Create(VkDevice device, VkPipelineLayout computeLayout, const std::wstring& shaderPath);
	void Create(VkDevice device, const PipelineLayout& computeLayout, const std::wstring& shaderPath)
	{
		Create(device, computeLayout.Get(), shaderPath);
	}

	void Bind(const VKCommandBuffer& computeBuffer) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> _createComputePipeline(
		VkDevice device, VkPipelineLayout computeLayout, const std::wstring& shaderPath
	) const noexcept;

private:
	std::unique_ptr<VkPipelineObject> m_computePipeline;

public:
	ComputePipeline(const ComputePipeline&) = delete;
	ComputePipeline& operator=(const ComputePipeline&) = delete;

	ComputePipeline(ComputePipeline&& other) noexcept
		: m_computePipeline{ std::move(other.m_computePipeline) }
	{}
	ComputePipeline& operator=(ComputePipeline&& other) noexcept
	{
		m_computePipeline = std::move(other.m_computePipeline);

		return *this;
	}
};
#endif
