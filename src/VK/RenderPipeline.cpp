#include <cstring>
#include <cmath>

#include <RenderPipeline.hpp>
#include <Terra.hpp>

RenderPipeline::RenderPipeline(VkDevice device, std::uint32_t bufferCount) noexcept
	: m_commandBuffers{ device }, m_bufferCount{ bufferCount }, m_modelCount{ 0u } {}

void RenderPipeline::AddGraphicsPipelineObject(
	std::unique_ptr<VkPipelineObject> graphicsPSO
) noexcept {
	m_graphicsPSO = std::move(graphicsPSO);
}

void RenderPipeline::AddGraphicsPipelineLayout(
	std::unique_ptr<PipelineLayout> layout
) noexcept {
	m_graphicsPipelineLayout = std::move(layout);
}

void RenderPipeline::AddComputePipelineObject(
	std::unique_ptr<VkPipelineObject> computePSO
) noexcept {
	m_computePSO = std::move(computePSO);
}

void RenderPipeline::AddComputePipelineLayout(
	std::unique_ptr<PipelineLayout> computeLayout
) noexcept {
	m_computePipelineLayout = std::move(computeLayout);
}

void RenderPipeline::BindGraphicsPipeline(
	VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet graphicsDescriptorSet
) const noexcept {
	vkCmdBindPipeline(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPSO->GetPipeline()
	);

	VkDescriptorSet descSets[] = { graphicsDescriptorSet };

	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

void RenderPipeline::BindComputePipeline(
	VkCommandBuffer computeCmdBuffer, VkDescriptorSet computeDescriptorSet
) const noexcept {
	vkCmdBindPipeline(
		computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_computePSO->GetPipeline()
	);

	VkDescriptorSet descSets[] = { computeDescriptorSet };

	vkCmdBindDescriptorSets(
		computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		m_computePipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

void RenderPipeline::DispatchCompute(VkCommandBuffer computeCmdBuffer) const noexcept {
	vkCmdDispatch(
		computeCmdBuffer,
		static_cast<std::uint32_t>(std::ceil(m_modelCount / THREADBLOCKSIZE)), 1u, 1u
	);
}

void RenderPipeline::DrawModels(
	VkCommandBuffer graphicsCmdBuffer, VkDeviceSize frameIndex
) const noexcept {
	static constexpr auto strideSize =
		static_cast<std::uint32_t>(sizeof(VkDrawIndexedIndirectCommand));

	vkCmdDrawIndexedIndirect(
		graphicsCmdBuffer, m_commandBuffers.GetGPUResource(),
		m_commandBuffers.GetSubAllocationOffset(frameIndex), m_modelCount, strideSize
	);
}

void RenderPipeline::BindResourceToMemory(VkDevice device) {
	m_commandBuffers.BindResourceToMemory(device);
}

void RenderPipeline::CreateBuffers(VkDevice device) noexcept {
	const size_t commandBufferSize = sizeof(VkDrawIndexedIndirectCommand) * m_modelCount;

	m_commandBuffers.CreateResource(
		device, static_cast<VkDeviceSize>(commandBufferSize), m_bufferCount,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);
	m_commandBuffers.SetMemoryOffsetAndType(device);
}

void RenderPipeline::RecordIndirectArguments(
	const std::vector<std::shared_ptr<IModel>>& models
) noexcept {
	for (size_t index = 0u; index < std::size(models); ++index) {
		const auto& model = models[index];
		const auto u32Index = static_cast<std::uint32_t>(std::size(m_indirectCommands));

		const std::uint32_t indexCount = model->GetIndexCount();
		const std::uint32_t indexOffset = model->GetIndexOffset();

		VkDrawIndexedIndirectCommand command{};
		command.firstIndex = indexOffset;
		command.firstInstance = u32Index;
		command.indexCount = indexCount;
		command.instanceCount = 1u;
		command.vertexOffset = 0u;

		m_indirectCommands.emplace_back(command);
	}

	m_modelCount += static_cast<std::uint32_t>(std::size(models));
}

void RenderPipeline::CopyData() noexcept {
	std::uint8_t* uploadMemoryStart = Terra::Resources::uploadMemory->GetMappedCPUPtr();
	for (std::uint32_t bufferIndex = 0u; bufferIndex < m_bufferCount; ++bufferIndex)
		memcpy(
			uploadMemoryStart + m_commandBuffers.GetUploadMemoryOffset(bufferIndex),
			std::data(m_indirectCommands),
			sizeof(VkDrawIndexedIndirectCommand) * std::size(m_indirectCommands)
		);

	m_indirectCommands = std::vector<VkDrawIndexedIndirectCommand>();
}

void RenderPipeline::RecordCopy(VkCommandBuffer copyBuffer) noexcept {
	m_commandBuffers.RecordCopy(copyBuffer);
}

void RenderPipeline::ReleaseUploadResources() noexcept {
	m_commandBuffers.CleanUpUploadResource();
}

void RenderPipeline::AcquireOwnerShip(
	VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_commandBuffers.AcquireOwnership(
		cmdBuffer, srcQueueIndex, dstQueueIndex, VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
	);
}

void RenderPipeline::ReleaseOwnership(
	VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_commandBuffers.ReleaseOwnerShip(copyCmdBuffer, srcQueueIndex, dstQueueIndex);
}
