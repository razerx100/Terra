#include <RenderPipeline.hpp>
#include <ranges>
#include <algorithm>
#include <cstring>

#include <Terra.hpp>

RenderPipeline::RenderPipeline(
	VkDevice device, std::vector<std::uint32_t> queueFamilyIndices
) noexcept
	: m_modelBuffers{ device }, m_commandBuffers{ device },
	m_queueFamilyIndices{ std::move(queueFamilyIndices) }, m_bufferCount{ 0u } {}

void RenderPipeline::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

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

void RenderPipeline::BindGraphicsPipeline(
	VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
) const noexcept {
	vkCmdBindPipeline(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPSO->GetPipeline()
	);

	VkDescriptorSet descSets[] = { descriptorSet };

	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

void RenderPipeline::DrawModels(
	VkCommandBuffer graphicsCmdBuffer, VkDeviceSize frameIndex
) const noexcept {
	static auto strideSize = static_cast<std::uint32_t>(sizeof(VkDrawIndexedIndirectCommand));

	vkCmdDrawIndexedIndirect(
		graphicsCmdBuffer, m_commandBuffers.GetGPUResource(),
		m_commandBuffers.GetSubAllocationOffset(frameIndex),
		static_cast<std::uint32_t>(std::size(m_opaqueModels)), strideSize
	);
}

void RenderPipeline::BindResourceToMemory(VkDevice device) {
	m_modelBuffers.BindResourceToMemory(device);
	m_commandBuffers.BindResourceToMemory(device);
}

void RenderPipeline::UpdateModelData(VkDeviceSize frameIndex) const noexcept {
	size_t offset = 0u;
	constexpr size_t bufferStride = sizeof(ModelConstantBuffer);

	std::uint8_t* cpuWriteStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();
	const VkDeviceSize modelBuffersOffset = m_modelBuffers.GetMemoryOffset(frameIndex);

	for (auto& model : m_opaqueModels) {
		ModelConstantBuffer modelBuffer{};
		modelBuffer.textureIndex = model->GetTextureIndex();
		modelBuffer.uvInfo = model->GetUVInfo();
		modelBuffer.modelMatrix = model->GetModelMatrix();

		memcpy(cpuWriteStart + modelBuffersOffset + offset, &modelBuffer, bufferStride);

		offset += bufferStride;
	}
}

void RenderPipeline::CreateBuffers(VkDevice device, std::uint32_t bufferCount) noexcept {
	const size_t modelCount = std::size(m_opaqueModels);
	const size_t modelBufferSize = sizeof(ModelConstantBuffer) * modelCount;

	m_modelBuffers.CreateResource(
		device, static_cast<VkDeviceSize>(modelBufferSize), bufferCount,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);
	m_modelBuffers.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	DescriptorSetManager::AddDescriptorForBuffer(
		m_modelBuffers, bufferCount, 2u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT
	);

	const size_t commandBufferSize = sizeof(VkDrawIndexedIndirectCommand) * modelCount;

	m_commandBuffers.CreateResource(
		device, static_cast<VkDeviceSize>(commandBufferSize), bufferCount,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		m_queueFamilyIndices
	);
	m_commandBuffers.SetMemoryOffsetAndType(device);

	m_bufferCount = bufferCount;
}

void RenderPipeline::CopyData() noexcept {
	std::vector<VkDrawIndexedIndirectCommand> commandBuffer;
	for (size_t index = 0u; index < std::size(m_opaqueModels); ++index) {
		const auto& model = m_opaqueModels[index];
		const auto u32Index = static_cast<std::uint32_t>(index);

		const std::uint32_t indexCount = model->GetIndexCount();
		const std::uint32_t indexOffset = model->GetIndexOffset();

		VkDrawIndexedIndirectCommand command{};
		command.firstIndex = indexOffset;
		command.firstInstance = u32Index;
		command.indexCount = indexCount;
		command.instanceCount = 1u;
		command.vertexOffset = 0u;

		commandBuffer.emplace_back(command);
	}

	std::uint8_t* uploadMemoryStart = Terra::Resources::uploadMemory->GetMappedCPUPtr();
	for (std::uint32_t bufferIndex = 0u; bufferIndex < m_bufferCount; ++bufferIndex)
		memcpy(
			uploadMemoryStart + m_commandBuffers.GetUploadMemoryOffset(bufferIndex),
			std::data(commandBuffer),
			sizeof(VkDrawIndexedIndirectCommand) * std::size(commandBuffer)
		);
}

void RenderPipeline::RecordCopy(VkCommandBuffer copyBuffer) noexcept {
	m_commandBuffers.RecordCopy(copyBuffer);
}

void RenderPipeline::ReleaseUploadResources() noexcept {
	m_commandBuffers.CleanUpUploadResource();
}
