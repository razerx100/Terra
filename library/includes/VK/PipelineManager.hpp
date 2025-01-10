#ifndef PIPELINE_MANAGER_HPP_
#define PIPELINE_MANAGER_HPP_
#include <optional>
#include <ranges>
#include <concepts>
#include <algorithm>
#include <PipelineLayout.hpp>
#include <GraphicsPipelineVS.hpp>
#include <GraphicsPipelineMS.hpp>
#include <ComputePipeline.hpp>

template<typename Pipeline>
class PipelineManager
{
public:
	PipelineManager(VkDevice device)
		: m_device{ device }, m_pipelineLayout{ VK_NULL_HANDLE }, m_shaderPath{}, m_pipelines{}
	{}

	void SetPipelineLayout(VkPipelineLayout pipelineLayout) noexcept
	{
		m_pipelineLayout = pipelineLayout;
	}

	void SetShaderPath(std::wstring shaderPath) noexcept
	{
		m_shaderPath = std::move(shaderPath);
	}

	void BindPipeline(size_t index, const VKCommandBuffer& commandBuffer) const noexcept
	{
		m_pipelines[index].Bind(commandBuffer);
	}

	[[nodiscard]]
	std::optional<std::uint32_t> TryToGetPSOIndex(const ShaderName& shaderName) const noexcept
	{
		std::optional<std::uint32_t> oPSOIndex{};

		auto result = std::ranges::find_if(m_pipelines,
			[&shaderName](const Pipeline& pipeline)
			{
				return shaderName == pipeline.GetShaderName();
			});

		if (result != std::end(m_pipelines))
			oPSOIndex = static_cast<std::uint32_t>(std::distance(std::begin(m_pipelines), result));

		return oPSOIndex;
	}

	std::uint32_t AddGraphicsPipeline(
		const ShaderName& fragmentShader,
		VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat
	)
		requires !std::is_same_v<Pipeline, ComputePipeline>
	{
		const auto psoIndex = static_cast<std::uint32_t>(std::size(m_pipelines));

		Pipeline pipeline{};

		pipeline.Create(
			m_device, m_pipelineLayout, colourFormat, depthStencilFormat, m_shaderPath, fragmentShader
		);

		m_pipelines.emplace_back(std::move(pipeline));

		return psoIndex;
	}

	std::uint32_t AddComputePipeline(const ShaderName& computeShader)
		requires std::is_same_v<Pipeline, ComputePipeline>
	{
		const auto psoIndex = static_cast<std::uint32_t>(std::size(m_pipelines));

		Pipeline pipeline{};

		pipeline.Create(m_device, m_pipelineLayout, computeShader, m_shaderPath);

		m_pipelines.emplace_back(std::move(pipeline));

		return psoIndex;
	}

	void RecreateAllGraphicsPipelines(
		VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat
	)
		requires !std::is_same_v<Pipeline, ComputePipeline>
	{
		for (Pipeline& pipeline : m_pipelines)
			pipeline.Recreate(m_device, m_pipelineLayout, colourFormat, depthStencilFormat, m_shaderPath);
	}

	void RecreateAllComputePipelines()
		requires std::is_same_v<Pipeline, ComputePipeline>
	{
		for (Pipeline& pipeline : m_pipelines)
			pipeline.Recreate(m_device, m_pipelineLayout, m_shaderPath);
	}

	[[nodiscard]]
	VkPipelineLayout GetLayout() const noexcept { return m_pipelineLayout; }

private:
	VkDevice              m_device;
	VkPipelineLayout      m_pipelineLayout;
	std::wstring          m_shaderPath;
	std::vector<Pipeline> m_pipelines;

public:
	PipelineManager(const PipelineManager&) = delete;
	PipelineManager& operator=(const PipelineManager&) = delete;

	PipelineManager(PipelineManager&& other) noexcept
		: m_device{ other.m_device },
		m_pipelineLayout{ other.m_pipelineLayout },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_pipelines{ std::move(other.m_pipelines) }
	{}
	PipelineManager& operator=(PipelineManager&& other) noexcept
	{
		m_device          = other.m_device;
		m_pipelineLayout  = other.m_pipelineLayout;
		m_shaderPath      = std::move(other.m_shaderPath);
		m_pipelines       = std::move(other.m_pipelines);

		return *this;
	}
};
#endif
