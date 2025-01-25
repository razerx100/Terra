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
		: m_device{ device }, m_pipelineLayout{ VK_NULL_HANDLE }, m_shaderPath{}, m_pipelines{},
		m_overwritablePSOs{}
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

	void SetOverwritable(const ShaderName& shaderName) noexcept
	{
		std::optional<std::uint32_t> oPsoIndex = TryToGetPSOIndex(shaderName);

		if (oPsoIndex)
			m_overwritablePSOs[oPsoIndex.value()] = true;
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

			psoIndex = AddPipeline(std::move(pipeline));
		}

		m_overwritablePSOs[psoIndex] = false;

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

			psoIndex = AddPipeline(std::move(pipeline));
		}

		m_overwritablePSOs[psoIndex] = false;

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

	[[nodiscard]]
	const std::wstring& GetShaderPath() const noexcept { return m_shaderPath; }

private:
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

	[[nodiscard]]
	std::optional<std::uint32_t> FindFirstOverwritableIndex() const noexcept
	{
		auto result = std::ranges::find(m_overwritablePSOs, true);

		std::optional<std::uint32_t> foundIndex{};;

		if (result != std::end(m_overwritablePSOs))
			foundIndex = static_cast<std::uint32_t>(
				std::distance(std::begin(m_overwritablePSOs), result)
			);

		return foundIndex;
	}

	[[nodiscard]]
	std::uint32_t AddPipeline(Pipeline&& pipeline) noexcept
	{
		auto psoIndex                                   = std::numeric_limits<std::uint32_t>::max();
		std::optional<std::uint32_t> oOverwritableIndex = FindFirstOverwritableIndex();

		if (oOverwritableIndex)
		{
			psoIndex              = oOverwritableIndex.value();
			m_pipelines[psoIndex] = std::move(pipeline);
		}
		else
		{
			psoIndex = static_cast<std::uint32_t>(std::size(m_pipelines));

			m_pipelines.emplace_back(std::move(pipeline));
		}

		return psoIndex;
	}

private:
	VkDevice              m_device;
	VkPipelineLayout      m_pipelineLayout;
	std::wstring          m_shaderPath;
	std::vector<Pipeline> m_pipelines;
	std::vector<bool>     m_overwritablePSOs;

public:
	PipelineManager(const PipelineManager&) = delete;
	PipelineManager& operator=(const PipelineManager&) = delete;

	PipelineManager(PipelineManager&& other) noexcept
		: m_device{ other.m_device },
		m_pipelineLayout{ other.m_pipelineLayout },
		m_shaderPath{ std::move(other.m_shaderPath) },
		m_pipelines{ std::move(other.m_pipelines) },
		m_overwritablePSOs{ std::move(other.m_overwritablePSOs) }
	{}
	PipelineManager& operator=(PipelineManager&& other) noexcept
	{
		m_device           = other.m_device;
		m_pipelineLayout   = other.m_pipelineLayout;
		m_shaderPath       = std::move(other.m_shaderPath);
		m_pipelines        = std::move(other.m_pipelines);
		m_overwritablePSOs = std::move(other.m_overwritablePSOs);

		return *this;
	}
};
#endif
