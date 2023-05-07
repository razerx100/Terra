#include <RenderEngineMeshShader.hpp>

#include <Terra.hpp>

RenderEngineMeshShader::RenderEngineMeshShader(const Args& arguments)
	: RenderEngineBase{ arguments.device.value() },
	m_vertexManager{
		arguments.device.value(), arguments.bufferCount.value(),
		arguments.queueIndices.value()
	}, m_meshletBuffer{ arguments.device.value() },
	m_queueIndicesTG{ arguments.queueIndices.value() },
	m_bufferCount{ arguments.bufferCount.value() } {}

void RenderEngineMeshShader::AddRequiredExtensionFunctions() noexcept {
	GraphicsPipelineMeshShader::AddRequiredExtensionFunctions();
}

void RenderEngineMeshShader::RetrieveExtensionFunctions() {
	GraphicsPipelineMeshShader::RetrieveExtensionFunctions();
}

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

void RenderEngineMeshShader::AddMeshletModelSet(
	std::vector<MeshletModel>& meshletModels, const std::wstring& fragmentShader
) noexcept {
	auto graphicsPipeline = std::make_unique<GraphicsPipelineMeshShader>();

	graphicsPipeline->ConfigureGraphicsPipeline(fragmentShader);

	static std::uint32_t modelCount = 0u;

	for (size_t index = 0u; index < std::size(meshletModels); ++index) {
		std::vector<Meshlet>&& meshlets = std::move(meshletModels[index].meshlets);

		graphicsPipeline->AddModelDetails(
			static_cast<std::uint32_t>(std::size(meshlets)),
			static_cast<std::uint32_t>(std::size(m_meshlets)), modelCount
		);

		++modelCount;

		std::ranges::move(meshlets, std::back_inserter(m_meshlets));
	}

	if (!m_graphicsPipeline0)
		m_graphicsPipeline0 = std::move(graphicsPipeline);
	else
		m_graphicsPipelines.emplace_back(std::move(graphicsPipeline));
}

void RenderEngineMeshShader::AddGVerticesAndPrimIndices(
	VkDevice device, std::vector<Vertex>&& gVertices,
	std::vector<std::uint32_t>&& gVerticesIndices, std::vector<std::uint32_t>&& gPrimIndices
) noexcept {
	m_vertexManager.AddGVerticesAndPrimIndices(
		device, std::move(gVertices), std::move(gVerticesIndices), std::move(gPrimIndices)
	);
}

void RenderEngineMeshShader::ConstructPipelines() {
	VkDevice device = Terra::device->GetLogicalDevice();

	ConstructGraphicsPipelineLayout(device);
	CreateGraphicsPipelines(device, m_graphicsPipeline0, m_graphicsPipelines);
}

void RenderEngineMeshShader::UpdateModelBuffers(VkDeviceSize frameIndex) const noexcept {
	Terra::bufferManager->Update<true>(frameIndex);
}

void RenderEngineMeshShader::RecordDrawCommands(
	VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
) {
	// One Pipeline needs to be bound before Descriptors can be bound.
	m_graphicsPipeline0->BindGraphicsPipeline(graphicsCmdBuffer);
	BindGraphicsDescriptorSets(graphicsCmdBuffer, frameIndex);

	m_graphicsPipeline0->DrawModels(graphicsCmdBuffer);

	for (auto& graphicsPipeline : m_graphicsPipelines) {
		graphicsPipeline->BindGraphicsPipeline(graphicsCmdBuffer);
		graphicsPipeline->DrawModels(graphicsCmdBuffer);
	}
}

void RenderEngineMeshShader::BindResourcesToMemory(VkDevice device) {
	m_vertexManager.BindResourceToMemory(device);
	m_meshletBuffer.BindResourceToMemory(device);
}

void RenderEngineMeshShader::RecordCopy(VkCommandBuffer transferBuffer) noexcept {
	m_vertexManager.RecordCopy(transferBuffer);
	m_meshletBuffer.RecordCopy(transferBuffer);
}

void RenderEngineMeshShader::ReleaseUploadResources() noexcept {
	m_vertexManager.ReleaseUploadResources();
	m_meshletBuffer.CleanUpUploadResource();
}

void RenderEngineMeshShader::AcquireOwnerShipGraphics(
	VkCommandBuffer graphicsCmdBuffer
) noexcept {
	m_vertexManager.AcquireOwnerShips(graphicsCmdBuffer);
	m_meshletBuffer.AcquireOwnership(
		graphicsCmdBuffer, m_queueIndicesTG.transfer, m_queueIndicesTG.graphics,
		VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT
	);
}

void RenderEngineMeshShader::ReleaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept {
	m_vertexManager.ReleaseOwnerships(transferCmdBuffer);
	m_meshletBuffer.ReleaseOwnerShip(
		transferCmdBuffer, m_queueIndicesTG.transfer, m_queueIndicesTG.graphics
	);
}

void RenderEngineMeshShader::CreateBuffers(VkDevice device) noexcept {
	const auto meshletBufferSize =
		static_cast<VkDeviceSize>(sizeof(Meshlet) * std::size(m_meshlets));

	m_meshletBuffer.CreateResource(
		device, meshletBufferSize, 1u, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);
	m_meshletBuffer.SetMemoryOffsetAndType(device);

	DescriptorInfo meshletDescInfo{
		.bindingSlot = 9u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	auto meshletBufferInfos = m_meshletBuffer.GetDescBufferInfoSpread(m_bufferCount);

	Terra::graphicsDescriptorSet->AddBuffersSplit(
		meshletDescInfo, std::move(meshletBufferInfos), VK_SHADER_STAGE_MESH_BIT_EXT
	);
}
