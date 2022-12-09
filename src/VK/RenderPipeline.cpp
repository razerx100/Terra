#include <cstring>
#include <cmath>
#include <VkResourceBarriers.hpp>

#include <RenderPipeline.hpp>
#include <Terra.hpp>

RenderPipeline::RenderPipeline(
	VkDevice device, std::uint32_t bufferCount,
	std::vector<std::uint32_t> computeAndGraphicsQueueIndices
) noexcept
	: m_commandBuffer{ device }, m_culldataBuffer{ device }, m_counterBuffer{ device },
	m_bufferCount{ bufferCount }, m_modelCount{ 0u },
	m_computeAndGraphicsQueueIndices{ std::move(computeAndGraphicsQueueIndices) } {
	for (size_t _ = 0u; _ < bufferCount; ++_)
		m_argumentBuffers.emplace_back(VkArgumentResourceView{ device });
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

void RenderPipeline::DispatchCompute(
	VkCommandBuffer computeCmdBuffer, VkDeviceSize frameIndex
) const noexcept {
	vkCmdDispatch(
		computeCmdBuffer,
		static_cast<std::uint32_t>(std::ceil(m_modelCount / THREADBLOCKSIZE)), 1u, 1u
	);

	auto& argumentBuffer = m_argumentBuffers[frameIndex];
	VkBufferBarrier().AddExecutionBarrier(
		argumentBuffer.GetResource(), argumentBuffer.GetBufferSize(), 0u,
		VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_INDIRECT_COMMAND_READ_BIT
	).RecordBarriers(
		computeCmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT
	);
}

void RenderPipeline::DrawModels(
	VkCommandBuffer graphicsCmdBuffer, VkDeviceSize frameIndex
) const noexcept {
	static constexpr auto strideSize =
		static_cast<std::uint32_t>(sizeof(VkDrawIndexedIndirectCommand));

	auto& argumentBuffer = m_argumentBuffers[frameIndex];

	vkCmdDrawIndexedIndirectCount(
		graphicsCmdBuffer, argumentBuffer.GetResource(),
		argumentBuffer.GetBufferOffset(), argumentBuffer.GetResource(),
		argumentBuffer.GetCounterOffset(), m_modelCount, strideSize
	);
}

void RenderPipeline::BindResourceToMemory(VkDevice device) {
	m_commandBuffer.BindResourceToMemory(device);

	for (auto& argumentBuffer : m_argumentBuffers)
		argumentBuffer.BindResourceToMemory(device);

	m_culldataBuffer.BindResourceToMemory(device);
	m_counterBuffer.BindResourceToMemory(device);
}

void RenderPipeline::CreateBuffers(VkDevice device) noexcept {
	const VkDeviceSize commandBufferSize =
		static_cast<VkDeviceSize>(sizeof(VkDrawIndexedIndirectCommand) * m_modelCount);

	m_commandBuffer.CreateResource(
		device, commandBufferSize, 1u, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);
	m_commandBuffer.SetMemoryOffsetAndType(device);

	for (auto& argumentBuffer : m_argumentBuffers) {
		argumentBuffer.CreateResource(
			device, commandBufferSize, m_computeAndGraphicsQueueIndices
		);
		argumentBuffer.SetMemoryOffsetAndType(device, MemoryType::gpuOnly);
	}

	m_culldataBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(sizeof(CullingData)), 1u,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	);
	m_culldataBuffer.SetMemoryOffsetAndType(device);

	m_counterBuffer.CreateResource(device, 4u, 1u, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	m_counterBuffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	// Culling Buffer
	DescriptorInfo cullingDescInfo{
		.bindingSlot = 5u,
		.descriptorCount = 1u,
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
	};

	std::vector<VkDescriptorBufferInfo> cullingBufferInfos;

	for (size_t _ = 0u; _ < m_bufferCount; ++_) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_culldataBuffer.GetGPUResource();
		bufferInfo.offset = m_culldataBuffer.GetFirstSubAllocationOffset();
		bufferInfo.range = m_culldataBuffer.GetSubAllocationSize();

		cullingBufferInfos.emplace_back(std::move(bufferInfo));
	}

	Terra::computeDescriptorSet->AddSetLayout(
		cullingDescInfo, VK_SHADER_STAGE_COMPUTE_BIT, std::move(cullingBufferInfos)
	);

	// Input Buffer
	DescriptorInfo inputDescInfo{
		.bindingSlot = 2u,
		.descriptorCount = 1u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	std::vector<VkDescriptorBufferInfo> inputBufferInfos;

	for (size_t _ = 0u; _ < m_bufferCount; ++_) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_commandBuffer.GetGPUResource();
		bufferInfo.offset = m_commandBuffer.GetFirstSubAllocationOffset();
		bufferInfo.range = m_commandBuffer.GetSubAllocationSize();

		inputBufferInfos.emplace_back(std::move(bufferInfo));
	}

	Terra::computeDescriptorSet->AddSetLayout(
		inputDescInfo, VK_SHADER_STAGE_COMPUTE_BIT, std::move(inputBufferInfos)
	);

	// Output Buffer
	DescriptorInfo outputDescInfo{
		.bindingSlot = 3u,
		.descriptorCount = 1u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	std::vector<VkDescriptorBufferInfo> outputBufferInfos;

	for (size_t index = 0u; index < m_bufferCount; ++index) {
		auto& argumentBuffer = m_argumentBuffers[index];

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = argumentBuffer.GetResource();
		bufferInfo.offset = argumentBuffer.GetBufferOffset();
		bufferInfo.range = argumentBuffer.GetResourceBufferSize();

		outputBufferInfos.emplace_back(std::move(bufferInfo));
	}

	Terra::computeDescriptorSet->AddSetLayout(
		outputDescInfo, VK_SHADER_STAGE_COMPUTE_BIT, std::move(outputBufferInfos)
	);

	// Counter Buffer
	DescriptorInfo counterDescInfo{
		.bindingSlot = 4u,
		.descriptorCount = 1u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	std::vector<VkDescriptorBufferInfo> counterBufferInfos;

	for (size_t index = 0u; index < m_bufferCount; ++index) {
		auto& argumentBuffer = m_argumentBuffers[index];

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = argumentBuffer.GetResource();
		bufferInfo.offset = argumentBuffer.GetCounterOffset();
		bufferInfo.range = argumentBuffer.GetCounterBufferSize();

		counterBufferInfos.emplace_back(std::move(bufferInfo));
	}

	Terra::computeDescriptorSet->AddSetLayout(
		counterDescInfo, VK_SHADER_STAGE_COMPUTE_BIT, std::move(counterBufferInfos)
	);
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
			uploadMemoryStart + m_commandBuffer.GetFirstUploadMemoryOffset(),
			std::data(m_indirectCommands),
			sizeof(VkDrawIndexedIndirectCommand) * std::size(m_indirectCommands)
		);

	// copy the culling data to the buffer.
	std::uint8_t* cullingBufferPtr =
		uploadMemoryStart + m_culldataBuffer.GetFirstUploadMemoryOffset();

	CullingData cullingData{};
	cullingData.commandCount = static_cast<std::uint32_t>(std::size(m_indirectCommands));
	cullingData.xBounds = XBOUNDS;
	cullingData.yBounds = YBOUNDS;
	cullingData.zBounds = ZBOUNDS;

	memcpy(cullingBufferPtr, &cullingData, sizeof(CullingData));

	// copy zero to counter buffer
	std::uint8_t* cpuWriteMemoryStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();

	std::uint8_t* counterCPUPtr = cpuWriteMemoryStart + m_counterBuffer.GetFirstMemoryOffset();
	const std::uint32_t zeroValue = 0u;
	memcpy(counterCPUPtr, &zeroValue, sizeof(std::uint32_t));

	m_indirectCommands = std::vector<VkDrawIndexedIndirectCommand>();
}

void RenderPipeline::RecordCopy(VkCommandBuffer copyBuffer) noexcept {
	m_commandBuffer.RecordCopy(copyBuffer);
}

void RenderPipeline::ReleaseUploadResources() noexcept {
	m_commandBuffer.CleanUpUploadResource();
}

void RenderPipeline::AcquireOwnerShip(
	VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_commandBuffer.AcquireOwnership(
		cmdBuffer, srcQueueIndex, dstQueueIndex, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	);
	m_culldataBuffer.AcquireOwnership(
		cmdBuffer, srcQueueIndex, dstQueueIndex, VK_ACCESS_SHADER_READ_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	);
}

void RenderPipeline::ReleaseOwnership(
	VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_commandBuffer.ReleaseOwnerShip(copyCmdBuffer, srcQueueIndex, dstQueueIndex);
	m_culldataBuffer.ReleaseOwnerShip(copyCmdBuffer, srcQueueIndex, dstQueueIndex);
}

void RenderPipeline::ResetCounterBuffer(
	VkCommandBuffer computeBuffer, VkDeviceSize frameIndex
) noexcept {
	auto& argumentBuffer = m_argumentBuffers[frameIndex];
	VkBufferCopy bufferInfo{
		.srcOffset = m_counterBuffer.GetFirstSubAllocationOffset(),
		.dstOffset = argumentBuffer.GetCounterOffset(),
		.size = argumentBuffer.GetCounterBufferSize()
	};

	vkCmdCopyBuffer(
		computeBuffer, m_counterBuffer.GetResource(), argumentBuffer.GetResource(), 1u,
		&bufferInfo
	);

	VkBufferBarrier().AddExecutionBarrier(
		argumentBuffer.GetResource(), argumentBuffer.GetCounterBufferSize(),
		argumentBuffer.GetCounterOffset(), VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_ACCESS_SHADER_READ_BIT
	).RecordBarriers(
		computeBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	);
}
