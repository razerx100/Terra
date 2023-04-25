#ifndef VK_PIPELINE_OBJECT_HPP_
#define VK_PIPELINE_OBJECT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VertexLayout.hpp>

class VkPipelineObject {
public:
	VkPipelineObject(VkDevice device) noexcept;
	~VkPipelineObject() noexcept;

	void CreateGraphicsPipelineVS(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const VertexLayout& vertexInput, VkShaderModule vertexShader,
		VkShaderModule fragmentShader
	) noexcept;
	void CreateGraphicsPipelineMS(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		VkShaderModule meshShader
	) noexcept;
	void CreateComputePipeline(
		VkDevice device, VkPipelineLayout computeLayout, VkShaderModule computeShader
	) noexcept;

	[[nodiscard]]
	VkPipeline GetPipeline() const noexcept;

private:
	[[nodiscard]]
	static VkPipelineShaderStageCreateInfo GetShaderStageInfo(
		VkShaderModule shader, VkShaderStageFlagBits shaderType
	) noexcept;

private:
	VkDevice m_deviceRef;
	VkPipeline m_pipeline;

private:
	class GraphicsPipelineCreateInfo {
	public:
		GraphicsPipelineCreateInfo(
			VkPipelineLayout graphicsLayout, VkRenderPass renderPass
		) noexcept;

		void SetInputAssembler(const VertexLayout& vertexLayout) noexcept;
		void SetVertexShaderStage(
			VkShaderModule vertexShader, VkShaderModule fragmentShader
		) noexcept;
		void SetMeshShaderStage(VkShaderModule meshShader) noexcept;

		[[nodiscard]]
		VkGraphicsPipelineCreateInfo const* GetGraphicsCreateInfoRef() const noexcept;

	private:
		void AddShaderStages() noexcept;

	private:
		VertexLayout m_vertexLayout;
		VkPipelineInputAssemblyStateCreateInfo m_inputAssemblerInfo;
		VkPipelineViewportStateCreateInfo m_viewportInfo;
		VkPipelineRasterizationStateCreateInfo m_rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo m_multisampleInfo;
		VkPipelineColorBlendAttachmentState m_colourAttachmentState;
		VkPipelineColorBlendStateCreateInfo m_colourStateInfo;
		std::vector<VkDynamicState> m_dynamicStates;
		VkPipelineDynamicStateCreateInfo m_dynamicStateInfo;
		std::vector<VkPipelineShaderStageCreateInfo> m_shaderStagesInfo;
		VkPipelineDepthStencilStateCreateInfo m_depthStencilStateInfo;
		VkGraphicsPipelineCreateInfo m_pipelineCreateInfo;
	};
};
#endif
