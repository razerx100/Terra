#include <cstring>
#include <cmath>
#include <VkResourceBarriers.hpp>
#include <Shader.hpp>

#include <RenderPipelineIndirectDraw.hpp>
#include <Terra.hpp>

RenderPipelineIndirectDraw::RenderPipelineIndirectDraw(
	VkDevice device, std::uint32_t bufferCount,
	std::vector<std::uint32_t> computeAndGraphicsQueueIndices
) noexcept
	: m_commandBuffer{ device }, m_culldataBuffer{ device }, m_counterBuffer{ device },
	m_bufferCount{ bufferCount }, m_modelCount{ 0u },
	m_computeAndGraphicsQueueIndices{ std::move(computeAndGraphicsQueueIndices) } {
	for (size_t _ = 0u; _ < bufferCount; ++_)
		m_argumentBuffers.emplace_back(VkArgumentResourceView{ device });
}

void RenderPipelineIndirectDraw::BindGraphicsPipeline(
	VkCommandBuffer graphicsCmdBuffer
) const noexcept {
	vkCmdBindPipeline(
		graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPSO->GetPipeline()
	);
}

void RenderPipelineIndirectDraw::DispatchCompute(
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

void RenderPipelineIndirectDraw::DrawModels(
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

void RenderPipelineIndirectDraw::BindResourceToMemory(VkDevice device) {
	m_commandBuffer.BindResourceToMemory(device);

	for (auto& argumentBuffer : m_argumentBuffers)
		argumentBuffer.BindResourceToMemory(device);

	m_culldataBuffer.BindResourceToMemory(device);
	m_counterBuffer.BindResourceToMemory(device);
}

void RenderPipelineIndirectDraw::CreateBuffers(VkDevice device) noexcept {
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
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
	};

	auto cullingBufferInfos = m_culldataBuffer.GetDescBufferInfoSpread(m_bufferCount);

	Terra::computeDescriptorSet->AddBuffersSplit(
		cullingDescInfo, std::move(cullingBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);

	// Input Buffer
	DescriptorInfo inputDescInfo{
		.bindingSlot = 2u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	auto inputBufferInfos = m_commandBuffer.GetDescBufferInfoSpread(m_bufferCount);

	Terra::computeDescriptorSet->AddBuffersSplit(
		inputDescInfo, std::move(inputBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);

	// Output Buffer
	DescriptorInfo outputDescInfo{
		.bindingSlot = 3u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	auto [outputBufferInfos, counterBufferInfos] = VkArgumentResourceView::GetDescBufferInfo(
		m_bufferCount, m_argumentBuffers
	);

	Terra::computeDescriptorSet->AddBuffersSplit(
		outputDescInfo, std::move(outputBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);

	// Counter Buffer
	DescriptorInfo counterDescInfo{
		.bindingSlot = 4u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	Terra::computeDescriptorSet->AddBuffersSplit(
		counterDescInfo, std::move(counterBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);
}

void RenderPipelineIndirectDraw::RecordIndirectArguments(
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

void RenderPipelineIndirectDraw::CopyData() noexcept {
	std::uint8_t* uploadMemoryStart = Terra::Resources::uploadMemory->GetMappedCPUPtr();
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

void RenderPipelineIndirectDraw::RecordCopy(VkCommandBuffer copyBuffer) noexcept {
	m_commandBuffer.RecordCopy(copyBuffer);
	m_culldataBuffer.RecordCopy(copyBuffer);
}

void RenderPipelineIndirectDraw::ReleaseUploadResources() noexcept {
	m_commandBuffer.CleanUpUploadResource();
	m_culldataBuffer.CleanUpUploadResource();
}

void RenderPipelineIndirectDraw::AcquireOwnerShip(
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

void RenderPipelineIndirectDraw::ReleaseOwnership(
	VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
) noexcept {
	m_commandBuffer.ReleaseOwnerShip(copyCmdBuffer, srcQueueIndex, dstQueueIndex);
	m_culldataBuffer.ReleaseOwnerShip(copyCmdBuffer, srcQueueIndex, dstQueueIndex);
}

void RenderPipelineIndirectDraw::ResetCounterBuffer(
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

std::unique_ptr<VkPipelineObject> RenderPipelineIndirectDraw::CreateGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader
) const noexcept {
	auto vs = std::make_unique<Shader>(device);
	vs->CreateShader(device, shaderPath + L"VertexShader.spv");

	auto fs = std::make_unique<Shader>(device);
	fs->CreateShader(device, shaderPath + fragmentShader);

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateGraphicsPipeline(
		device, graphicsLayout, renderPass,
		VertexLayout()
		.AddInput(VK_FORMAT_R32G32B32_SFLOAT, 12u)
		.AddInput(VK_FORMAT_R32G32_SFLOAT, 8u)
		.InitLayout(),
		vs->GetByteCode(), fs->GetByteCode()
	);

	return pso;
}

void RenderPipelineIndirectDraw::ConfigureGraphicsPipelineObject(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath, const std::wstring& fragmentShader
) noexcept {
	m_graphicsPSO = CreateGraphicsPipeline(
		device, graphicsLayout, renderPass, shaderPath, fragmentShader
	);
}
