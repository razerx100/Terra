#ifndef VK_PIPELINE_OBJECT_HPP_
#define VK_PIPELINE_OBJECT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <utility>
#include <VertexLayout.hpp>

class PipelineBuilderBase
{
protected:
	[[nodiscard]]
	static VkPipelineShaderStageCreateInfo GetShaderStageInfo(
		VkShaderModule shader, VkShaderStageFlagBits shaderType
	) noexcept;
};

class StencilOpStateBuilder
{
public:
	StencilOpStateBuilder()
		: m_opState{
			.failOp      = VK_STENCIL_OP_KEEP,
			.passOp      = VK_STENCIL_OP_KEEP,
			.depthFailOp = VK_STENCIL_OP_KEEP,
			.compareOp   = VK_COMPARE_OP_ALWAYS
		}
	{}

	StencilOpStateBuilder& StencilOps(
		VkStencilOp failOp, VkStencilOp passOp, VkStencilOp depthFailOp
	) noexcept {
		m_opState.failOp      = failOp;
		m_opState.passOp      = passOp;
		m_opState.depthFailOp = depthFailOp;

		return *this;
	}

	StencilOpStateBuilder& CompareOp(VkCompareOp compareOp) noexcept
	{
		m_opState.compareOp = compareOp;

		return *this;
	}

	StencilOpStateBuilder& Masks(std::uint32_t compareMask, std::uint32_t writeMask) noexcept
	{
		m_opState.compareMask = compareMask;
		m_opState.writeMask   = writeMask;

		return *this;
	}

	StencilOpStateBuilder& Reference(std::uint32_t reference) noexcept
	{
		m_opState.reference = reference;

		return *this;
	}

	[[nodiscard]]
	VkStencilOpState Get() const noexcept { return m_opState; }

private:
	VkStencilOpState m_opState;
};

class DepthStencilStateBuilder
{
public:
	DepthStencilStateBuilder()
		: m_depthStencilStateInfo{
			.sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable       = VK_TRUE,
			.depthWriteEnable      = VK_TRUE,
			.depthCompareOp        = VK_COMPARE_OP_LESS,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable     = VK_FALSE,
			.front                 = { StencilOpStateBuilder{}.Get() },
			.back                  = { StencilOpStateBuilder{}.Get() },
			.minDepthBounds        = 0.f,
			.maxDepthBounds        = 1.f
		}
	{}

	DepthStencilStateBuilder& Enable(
		VkBool32 depthTest, VkBool32 depthWrite, VkBool32 depthBoundsTest, VkBool32 stencilTest
	) noexcept {
		m_depthStencilStateInfo.depthTestEnable       = depthTest;
		m_depthStencilStateInfo.depthWriteEnable      = depthWrite;
		m_depthStencilStateInfo.depthBoundsTestEnable = depthBoundsTest;
		m_depthStencilStateInfo.stencilTestEnable     = stencilTest;

		return *this;
	}

	DepthStencilStateBuilder& DepthCompareOp(VkCompareOp compareOp) noexcept
	{
		m_depthStencilStateInfo.depthCompareOp = compareOp;

		return *this;
	}

	DepthStencilStateBuilder& DepthBounds(float minBounds, float maxBounds) noexcept
	{
		m_depthStencilStateInfo.minDepthBounds = minBounds;
		m_depthStencilStateInfo.maxDepthBounds = maxBounds;

		return *this;
	}

	DepthStencilStateBuilder& StencilOpStates(
		const StencilOpStateBuilder& front, const StencilOpStateBuilder& back
	) noexcept {
		m_depthStencilStateInfo.front = front.Get();
		m_depthStencilStateInfo.back  = back.Get();

		return *this;
	}

	[[nodiscard]]
	VkPipelineDepthStencilStateCreateInfo const* GetRef() const noexcept
	{
		return &m_depthStencilStateInfo;
	}
	[[nodiscard]]
	VkPipelineDepthStencilStateCreateInfo Get() const noexcept { return m_depthStencilStateInfo; }

private:
	VkPipelineDepthStencilStateCreateInfo m_depthStencilStateInfo;
};

class GraphicsPipelineBuilder : private PipelineBuilderBase
{
public:
	GraphicsPipelineBuilder(VkPipelineLayout graphicsLayout, VkRenderPass renderPass)
		: m_vertexLayout{},
		m_inputAssemblerInfo{
			.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		}, m_viewportInfo{
			.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1u,
			.pViewports    = nullptr,
			.scissorCount  = 1u,
			.pScissors     = nullptr
		}, m_rasterizationInfo{
			.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable        = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode             = VK_POLYGON_MODE_FILL,
			.cullMode                = VK_CULL_MODE_BACK_BIT,
			.frontFace               = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable         = VK_FALSE,
			.depthBiasConstantFactor = 0.f,
			.depthBiasClamp          = 0.f,
			.depthBiasSlopeFactor    = 0.f,
			.lineWidth               = 1.f
		}, m_multisampleInfo{
			.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable   = VK_FALSE,
			.minSampleShading      = 1.f,
			.pSampleMask           = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable      = VK_FALSE
		}, m_colourAttachmentState{
			.blendEnable         = VK_FALSE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
			.colorBlendOp        = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp        = VK_BLEND_OP_ADD,
			.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
				| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		}, m_colourStateInfo{
			.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable   = VK_FALSE,
			.logicOp         = VK_LOGIC_OP_COPY,
			.attachmentCount = 1u,
			.pAttachments    = &m_colourAttachmentState,
			.blendConstants  = { 0.f, 0.f, 0.f, 0.f }
		}, m_dynamicStates{
			{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }
		}, m_dynamicStateInfo{
			.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<std::uint32_t>(std::size(m_dynamicStates)),
			.pDynamicStates    = std::data(m_dynamicStates)
		}, m_depthStencilStateInfo{ DepthStencilStateBuilder{}.Get() },
		m_pipelineCreateInfo{
			.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.flags               = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
			.pViewportState      = &m_viewportInfo,
			.pRasterizationState = &m_rasterizationInfo,
			.pMultisampleState   = &m_multisampleInfo,
			.pDepthStencilState  = &m_depthStencilStateInfo,
			.pColorBlendState    = &m_colourStateInfo,
			.pDynamicState       = &m_dynamicStateInfo,
			.layout              = graphicsLayout,
			.renderPass          = renderPass,
			.subpass             = 0u,
			.basePipelineHandle  = VK_NULL_HANDLE,
			.basePipelineIndex   = -1
		}
	{}

	// Can only be set on a Vertex Shader based Pipeline.
	GraphicsPipelineBuilder& SetInputAssembler(
		const VertexLayout& vertexLayout,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VkBool32 primitiveRestart = VK_FALSE
	) noexcept;
	GraphicsPipelineBuilder& SetVertexStage(
		VkShaderModule vertexShader, VkShaderModule fragmentShader
	) noexcept;

	GraphicsPipelineBuilder& SetDepthStencilState(
		const DepthStencilStateBuilder& depthStencilBuilder
	) noexcept;

	// Can only be set on a Mesh Shader based Pipeline.
	GraphicsPipelineBuilder& SetMeshStage(
		VkShaderModule meshShader, VkShaderModule fragmentShader
	) noexcept;
	GraphicsPipelineBuilder& SetTaskStage(VkShaderModule taskShader) noexcept;

	GraphicsPipelineBuilder& AddDynamicState(VkDynamicState dynamicState) noexcept;

	// Will add more Builders for each type when I want to customise them.

	[[nodiscard]]
	VkGraphicsPipelineCreateInfo const* GetRef() const noexcept { return &m_pipelineCreateInfo; }
	[[nodiscard]]
	VkGraphicsPipelineCreateInfo Get() const noexcept { return m_pipelineCreateInfo; }

private:
	void UpdateShaderStages() noexcept;
	// Since the Mesh Shader pipeline can't have an Input Assembler, can't add those
	// if the Pipeline is for Mesh Shader.
	void UpdatePointers(bool inputAssembler) noexcept;

private:
	VertexLayout                                 m_vertexLayout;
	VkPipelineInputAssemblyStateCreateInfo       m_inputAssemblerInfo;
	VkPipelineViewportStateCreateInfo            m_viewportInfo;
	VkPipelineRasterizationStateCreateInfo       m_rasterizationInfo;
	VkPipelineMultisampleStateCreateInfo         m_multisampleInfo;
	VkPipelineColorBlendAttachmentState          m_colourAttachmentState;
	VkPipelineColorBlendStateCreateInfo          m_colourStateInfo;
	std::vector<VkDynamicState>                  m_dynamicStates;
	VkPipelineDynamicStateCreateInfo             m_dynamicStateInfo;
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStagesInfo;
	VkPipelineDepthStencilStateCreateInfo        m_depthStencilStateInfo;
	VkGraphicsPipelineCreateInfo                 m_pipelineCreateInfo;

public:
	GraphicsPipelineBuilder(const GraphicsPipelineBuilder& other) noexcept
		: m_vertexLayout{ other.m_vertexLayout },
		m_inputAssemblerInfo{ other.m_inputAssemblerInfo },
		m_viewportInfo{ other.m_viewportInfo },
		m_rasterizationInfo{ other.m_rasterizationInfo },
		m_multisampleInfo{ other.m_multisampleInfo },
		m_colourAttachmentState{ other.m_colourAttachmentState },
		m_colourStateInfo{ other.m_colourStateInfo },
		m_dynamicStates{ other.m_dynamicStates },
		m_dynamicStateInfo{ other.m_dynamicStateInfo },
		m_shaderStagesInfo{ other.m_shaderStagesInfo },
		m_depthStencilStateInfo{ other.m_depthStencilStateInfo },
		m_pipelineCreateInfo{ other.m_pipelineCreateInfo }
	{
		UpdatePointers(other.m_pipelineCreateInfo.pInputAssemblyState);
	}
	GraphicsPipelineBuilder& operator=(const GraphicsPipelineBuilder& other) noexcept
	{
		m_vertexLayout          = other.m_vertexLayout;
		m_inputAssemblerInfo    = other.m_inputAssemblerInfo;
		m_viewportInfo          = other.m_viewportInfo;
		m_rasterizationInfo     = other.m_rasterizationInfo;
		m_multisampleInfo       = other.m_multisampleInfo;
		m_colourAttachmentState = other.m_colourAttachmentState;
		m_colourStateInfo       = other.m_colourStateInfo;
		m_dynamicStates         = other.m_dynamicStates;
		m_dynamicStateInfo      = other.m_dynamicStateInfo;
		m_shaderStagesInfo      = other.m_shaderStagesInfo;
		m_depthStencilStateInfo = other.m_depthStencilStateInfo;
		m_pipelineCreateInfo    = other.m_pipelineCreateInfo;

		UpdatePointers(other.m_pipelineCreateInfo.pInputAssemblyState);

		return *this;
	}
	GraphicsPipelineBuilder(GraphicsPipelineBuilder&& other) noexcept
		: m_vertexLayout{ other.m_vertexLayout },
		m_inputAssemblerInfo{ other.m_inputAssemblerInfo },
		m_viewportInfo{ other.m_viewportInfo },
		m_rasterizationInfo{ other.m_rasterizationInfo },
		m_multisampleInfo{ other.m_multisampleInfo },
		m_colourAttachmentState{ other.m_colourAttachmentState },
		m_colourStateInfo{ other.m_colourStateInfo },
		m_dynamicStates{ std::move(other.m_dynamicStates) },
		m_dynamicStateInfo{ other.m_dynamicStateInfo },
		m_shaderStagesInfo{ std::move(other.m_shaderStagesInfo) },
		m_depthStencilStateInfo{ other.m_depthStencilStateInfo },
		m_pipelineCreateInfo{ other.m_pipelineCreateInfo }
	{
		UpdatePointers(other.m_pipelineCreateInfo.pInputAssemblyState);
	}
	GraphicsPipelineBuilder& operator=(GraphicsPipelineBuilder&& other) noexcept
	{
		m_vertexLayout          = other.m_vertexLayout;
		m_inputAssemblerInfo    = other.m_inputAssemblerInfo;
		m_viewportInfo          = other.m_viewportInfo;
		m_rasterizationInfo     = other.m_rasterizationInfo;
		m_multisampleInfo       = other.m_multisampleInfo;
		m_colourAttachmentState = other.m_colourAttachmentState;
		m_colourStateInfo       = other.m_colourStateInfo;
		m_dynamicStates         = std::move(other.m_dynamicStates);
		m_dynamicStateInfo      = other.m_dynamicStateInfo;
		m_shaderStagesInfo      = std::move(other.m_shaderStagesInfo);
		m_depthStencilStateInfo = other.m_depthStencilStateInfo;
		m_pipelineCreateInfo    = other.m_pipelineCreateInfo;

		UpdatePointers(other.m_pipelineCreateInfo.pInputAssemblyState);

		return *this;
	}
};

class ComputePipelineBuilder : private PipelineBuilderBase
{
public:
	ComputePipelineBuilder(VkPipelineLayout computeLayout)
		: m_pipelineCreateInfo{
			.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.flags              = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
			.layout             = computeLayout,
			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex  = -1
		}
	{}

	ComputePipelineBuilder& SetComputeStage(VkShaderModule computeShader) noexcept;

	[[nodiscard]]
	VkComputePipelineCreateInfo const* GetRef() const noexcept { return &m_pipelineCreateInfo; }
	[[nodiscard]]
	VkComputePipelineCreateInfo Get() const noexcept { return m_pipelineCreateInfo; }

private:
	VkComputePipelineCreateInfo m_pipelineCreateInfo;

public:
	ComputePipelineBuilder(const ComputePipelineBuilder& other) noexcept
		: m_pipelineCreateInfo{ other.m_pipelineCreateInfo }
	{}
	ComputePipelineBuilder& operator=(const ComputePipelineBuilder& other) noexcept
	{
		m_pipelineCreateInfo = other.m_pipelineCreateInfo;

		return *this;
	}
	ComputePipelineBuilder(ComputePipelineBuilder&& other) noexcept
		: m_pipelineCreateInfo{ other.m_pipelineCreateInfo }
	{}
	ComputePipelineBuilder& operator=(ComputePipelineBuilder&& other) noexcept
	{
		m_pipelineCreateInfo = other.m_pipelineCreateInfo;

		return *this;
	}
};

class VkPipelineObject
{
public:
	VkPipelineObject(VkDevice device) noexcept : m_device{ device }, m_pipeline{ VK_NULL_HANDLE } {}
	~VkPipelineObject() noexcept;

	void CreateGraphicsPipeline(const GraphicsPipelineBuilder& builder);
	void CreateComputePipeline(const ComputePipelineBuilder& builder);

	[[nodiscard]]
	VkPipeline Get() const noexcept { return m_pipeline; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice   m_device;
	VkPipeline m_pipeline;

public:
	VkPipelineObject(const VkPipelineObject&) = delete;
	VkPipelineObject& operator=(const VkPipelineObject&) = delete;

	VkPipelineObject(VkPipelineObject&& other) noexcept
		: m_device{ other.m_device }, m_pipeline{ std::exchange(other.m_pipeline, VK_NULL_HANDLE) }
	{}
	VkPipelineObject& operator=(VkPipelineObject&& other) noexcept
	{
		SelfDestruct();

		m_device   = other.m_device;
		m_pipeline = std::exchange(other.m_pipeline, VK_NULL_HANDLE);

		return *this;
	}
};
#endif
