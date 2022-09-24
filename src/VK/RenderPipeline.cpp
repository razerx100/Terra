#include <RenderPipeline.hpp>
#include <ranges>
#include <algorithm>
#include <cstring>

#include <Terra.hpp>

RenderPipeline::RenderPipeline(VkDevice device) noexcept : m_modelBuffers{ device } {}

void RenderPipeline::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void RenderPipeline::AddGraphicsPipelineObject(
	std::unique_ptr<PipelineObjectGFX> pso
) noexcept {
	m_graphicsPSO = std::move(pso);
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
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPSO->GetPipelineObject()
	);

	VkDescriptorSet descSets[] = { descriptorSet };

	vkCmdBindDescriptorSets(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_graphicsPipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

// Need to set model index as push constant
void RenderPipeline::DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept {
	for (size_t index = 0u; index < std::size(m_opaqueModels); ++index) {
		const auto& model = m_opaqueModels[index];
		const auto u32Index = static_cast<std::uint32_t>(index);

		vkCmdPushConstants(
			graphicsCmdBuffer, m_graphicsPipelineLayout->GetLayout(),
			VK_SHADER_STAGE_VERTEX_BIT, 0u, 4u, &u32Index
		);

		const std::uint32_t indexCount = model->GetIndexCount();
		const std::uint32_t indexOffset = model->GetIndexOffset();

		vkCmdDrawIndexed(graphicsCmdBuffer, indexCount, 1u, indexOffset, 0u, 0u);
	}
}

void RenderPipeline::BindResourceToMemory(VkDevice device) {
	m_modelBuffers.BindResourceToMemory(device);
}

void RenderPipeline::UpdateModelData(size_t frameIndex) const noexcept {
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
	size_t bufferSize = sizeof(ModelConstantBuffer) * std::size(m_opaqueModels);

	m_modelBuffers.CreateResource(
		device, bufferSize, bufferCount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);

	m_modelBuffers.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	DescriptorSetManager::AddDescriptorForBuffer(
		m_modelBuffers, bufferCount, 2u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_SHADER_STAGE_VERTEX_BIT
	);
}
