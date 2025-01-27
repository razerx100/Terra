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
#include <ReusableVector.hpp>

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
		m_pipelines.at(index).Bind(commandBuffer);
	}

	void SetOverwritable(const ShaderName& shaderName) noexcept
	{
		std::optional<std::uint32_t> oPsoIndex = TryToGetPSOIndex(shaderName);

		if (oPsoIndex)
			m_pipelines.MakeUnavailable(oPsoIndex.value());
	}

	std::uint32_t AddOrGetGraphicsPipeline(
		const ShaderName& fragmentShader, VkFormat colourFormat,
		const DepthStencilFormat& depthStencilFormat
	)
		requires !std::is_same_v<Pipeline, ComputePipeline>
	{
		auto psoIndex                          = std::numeric_limits<std::uint32_t>::max();
		std::optional<std::uint32_t> oPSOIndex = TryToGetPSOIndex(fragmentShader);

		if (oPSOIndex)
			psoIndex = oPSOIndex.value();
		else
		{
			Pipeline pipeline{};

			pipeline.Create(
				m_device, m_pipelineLayout, colourFormat, depthStencilFormat, m_shaderPath, fragmentShader
			);

			psoIndex = static_cast<std::uint32_t>(m_pipelines.Add(std::move(pipeline)));
		}

		return psoIndex;
	}

	std::uint32_t AddOrGetComputePipeline(const ShaderName& computeShader)
		requires std::is_same_v<Pipeline, ComputePipeline>
	{
		auto psoIndex                          = std::numeric_limits<std::uint32_t>::max();
		std::optional<std::uint32_t> oPSOIndex = TryToGetPSOIndex(computeShader);

		if (oPSOIndex)
			psoIndex = oPSOIndex.value();
		else
		{
			Pipeline pipeline{};

			pipeline.Create(m_device, m_pipelineLayout, computeShader, m_shaderPath);

			psoIndex = static_cast<std::uint32_t>(m_pipelines.Add(std::move(pipeline)));
		}

		return psoIndex;
	}

	void RecreateAllGraphicsPipelines(
		VkFormat colourFormat, const DepthStencilFormat& depthStencilFormat
	)
		requires !std::is_same_v<Pipeline, ComputePipeline>
	{
		std::vector<Pipeline>& pipelines = m_pipelines.Get();

		for (Pipeline& pipeline : pipelines)
			pipeline.Recreate(m_device, m_pipelineLayout, colourFormat, depthStencilFormat, m_shaderPath);
	}

	void RecreateAllComputePipelines()
		requires std::is_same_v<Pipeline, ComputePipeline>
	{
		std::vector<Pipeline>& pipelines = m_pipelines.Get();

		for (Pipeline& pipeline : pipelines)
			pipeline.Recreate(m_device, m_pipelineLayout, m_shaderPath);
	}

	[[nodiscard]]
	VkPipelineLayout GetLayout() const noexcept { return m_pipelineLayout; }

	[[nodiscard]]
	const std::wstring& GetShaderPath() const noexcept { return m_shaderPath; }

private:
	[[nodiscard]]
	std::optional<std::uint32_t> TryToGetPSOIndex(const ShaderName& shaderName) const noexcept
	{
		std::optional<std::uint32_t> oPSOIndex{};

		const std::vector<Pipeline>& pipelines = m_pipelines.Get();

		auto result = std::ranges::find_if(pipelines,
			[&shaderName](const Pipeline& pipeline)
			{
				return shaderName == pipeline.GetShaderName();
			});

		if (result != std::end(pipelines))
			oPSOIndex = static_cast<std::uint32_t>(std::distance(std::begin(pipelines), result));

		return oPSOIndex;
	}

private:
	VkDevice                 m_device;
	VkPipelineLayout         m_pipelineLayout;
	std::wstring             m_shaderPath;
	ReusableVector<Pipeline> m_pipelines;

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
		m_device         = other.m_device;
		m_pipelineLayout = other.m_pipelineLayout;
		m_shaderPath     = std::move(other.m_shaderPath);
		m_pipelines      = std::move(other.m_pipelines);

		return *this;
	}
};
#endif
