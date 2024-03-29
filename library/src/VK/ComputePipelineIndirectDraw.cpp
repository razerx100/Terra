#include <VkShader.hpp>
#include <VkResourceBarriers2.hpp>

#include <ComputePipelineIndirectDraw.hpp>
#include <Terra.hpp>

ComputePipelineIndirectDraw::ComputePipelineIndirectDraw(
	VkDevice device, std::uint32_t bufferCount,	QueueIndices3 queueIndices
) noexcept
	: m_commandBuffer{ device }, m_cullDataBuffer{ device }, m_counterResetBuffer{ device },
	m_queueIndices{ queueIndices }, m_bufferCount{ bufferCount }, m_modelCount{ 0u } {
	// Copy ctors are deleted. So, have to emplace_back to use move ctors
	for (size_t _ = 0u; _ < bufferCount; ++_) {
		m_argumentBuffers.emplace_back(VkResourceView{ device });
		m_counterBuffers.emplace_back(VkUploadableBufferResourceView{ device });
	}
}

void ComputePipelineIndirectDraw::CreateComputePipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) noexcept {
	m_computePipelineLayout = _createComputePipelineLayout(device, layoutCount, setLayouts);
}

void ComputePipelineIndirectDraw::CreateComputePipeline(
	VkDevice device, const std::wstring& shaderPath
) noexcept {
	m_computePipeline = _createComputePipeline(
		device, m_computePipelineLayout->GetLayout(), shaderPath
	);
}

std::unique_ptr<PipelineLayout> ComputePipelineIndirectDraw::_createComputePipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) const noexcept {
	auto pipelineLayout = std::make_unique<PipelineLayout>(device);

	// Push constants needs to be serialised according to the shader stages
	// Doesn't do anything different now but might, in the future idk

	pipelineLayout->CreateLayout(setLayouts, layoutCount);

	return pipelineLayout;
}

std::unique_ptr<VkPipelineObject> ComputePipelineIndirectDraw::_createComputePipeline(
	VkDevice device, VkPipelineLayout computeLayout, const std::wstring& shaderPath
) const noexcept {
	auto cs = std::make_unique<VkShader>(device);
	cs->CreateShader(device, shaderPath + L"ComputeShader.spv");

	auto pso = std::make_unique<VkPipelineObject>(device);
	pso->CreateComputePipeline(device, computeLayout, cs->GetShaderModule());

	return pso;
}

void ComputePipelineIndirectDraw::DispatchCompute(
	VkCommandBuffer computeCmdBuffer
) const noexcept {
	vkCmdDispatch(
		computeCmdBuffer,
		static_cast<std::uint32_t>(std::ceil(m_modelCount / THREADBLOCKSIZE)), 1u, 1u
	);
}

void ComputePipelineIndirectDraw::BindComputePipeline(
	VkCommandBuffer computeCmdBuffer, size_t frameIndex
) const noexcept {
	vkCmdBindPipeline(
		computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		m_computePipeline->GetPipeline()
	);

	VkDescriptorSet descSets[] = { Terra::Get().ComputeDesc().GetDescriptorSet(frameIndex)};
	vkCmdBindDescriptorSets(
		computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
		m_computePipelineLayout->GetLayout(), 0u, 1u,
		descSets, 0u, nullptr
	);
}

void ComputePipelineIndirectDraw::BindResourceToMemory(VkDevice device) {
	m_commandBuffer.BindResourceToMemory(device);

	for (auto& argumentBuffer : m_argumentBuffers)
		argumentBuffer.BindResourceToMemory(device);

	for (auto& counterBuffer : m_counterBuffers)
		counterBuffer.BindResourceToMemory(device);

	m_cullDataBuffer.BindResourceToMemory(device);
	m_counterResetBuffer.BindResourceToMemory(device);
}

void ComputePipelineIndirectDraw::CreateBuffers(VkDevice device) noexcept {
	const auto commandBufferSize =
		static_cast<VkDeviceSize>(sizeof(VkDrawIndexedIndirectCommand) * m_modelCount);

	m_commandBuffer.CreateResource(
		device, commandBufferSize, 1u, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);
	m_commandBuffer.SetMemoryOffsetAndType(device);

	Terra::Get().Res().UploadCont().AddMemory(
		std::data(m_indirectCommands),
		sizeof(VkDrawIndexedIndirectCommand) * std::size(m_indirectCommands),
		m_commandBuffer.GetFirstUploadMemoryOffset()
	);

	for (auto& argumentBuffer : m_argumentBuffers) {
		argumentBuffer.CreateResource(
			device, commandBufferSize, 1u,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			ResolveQueueIndices(m_queueIndices.compute, m_queueIndices.graphics)
		);
		argumentBuffer.SetMemoryOffsetAndType(device, MemoryType::gpuOnly);
	}

	const auto counterBufferSize =
		static_cast<VkDeviceSize>(COUNTERBUFFERSTRIDE * std::size(m_modelCountOffsets));

	for (auto& counterBuffer : m_counterBuffers) {
		counterBuffer.CreateResource(
			device, counterBufferSize, 1u,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			ResolveQueueIndices(
				m_queueIndices.compute, m_queueIndices.graphics, m_queueIndices.transfer
			)
		);
		counterBuffer.SetMemoryOffsetAndType(device);
	}

	m_cullDataBuffer.CreateResource(
		device, static_cast<VkDeviceSize>(sizeof(CullingData)), 1u,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	);
	m_cullDataBuffer.SetMemoryOffsetAndType(device);

	m_counterResetBuffer.CreateResource(device, 4u, 1u, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	m_counterResetBuffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	// Culling Buffer
	DescriptorInfo cullingDescInfo{
		.bindingSlot = 0u,
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
	};

	auto cullingBufferInfos = m_cullDataBuffer.GetDescBufferInfoSpread(m_bufferCount);

	DescriptorSetManager& computeDesc = Terra::Get().ComputeDesc();

	computeDesc.AddBuffersSplit(
		cullingDescInfo, std::move(cullingBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);

	// Input Buffer
	DescriptorInfo inputDescInfo{
		.bindingSlot = 1u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	auto inputBufferInfos = m_commandBuffer.GetDescBufferInfoSpread(m_bufferCount);

	computeDesc.AddBuffersSplit(
		inputDescInfo, std::move(inputBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);

	// Output Buffer
	DescriptorInfo outputDescInfo{
		.bindingSlot = 2u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};

	auto outputBufferInfos = VkResourceView::GetDescBufferInfoSplit(
		m_bufferCount, m_argumentBuffers
	);

	computeDesc.AddBuffersSplit(
		outputDescInfo, std::move(outputBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);

	// Counter Buffer
	DescriptorInfo counterDescInfo{
		.bindingSlot = 3u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};
	auto counterBufferInfos = VkResourceView::GetDescBufferInfoSplit(
		m_bufferCount, m_counterBuffers
	);

	computeDesc.AddBuffersSplit(
		counterDescInfo, std::move(counterBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);
}

void ComputePipelineIndirectDraw::RecordIndirectArguments(
	const std::vector<std::shared_ptr<IModel>>& models
) noexcept {
	for (size_t index = 0u; index < std::size(models); ++index) {
		const auto& model = models[index];
		const auto u32Index = static_cast<std::uint32_t>(std::size(m_indirectCommands));

		const std::uint32_t indexCount = model->GetIndexCount();
		const std::uint32_t indexOffset = model->GetIndexOffset();

		VkDrawIndexedIndirectCommand command{
			.indexCount = indexCount,
			.instanceCount = 1u,
			.firstIndex = indexOffset,
			.vertexOffset = 0u,
			.firstInstance = u32Index
		};

		m_indirectCommands.emplace_back(command);
	}

	m_modelCountOffsets.emplace_back(m_modelCount);
	m_modelCount += static_cast<std::uint32_t>(std::size(models));
}

void ComputePipelineIndirectDraw::CopyData() noexcept {
	std::uint8_t* uploadMemoryStart = Terra::Get().Res().Upload().GetMappedCPUPtr();
	// copy the culling data to the buffer.
	std::uint8_t* cullingBufferPtr =
		uploadMemoryStart + m_cullDataBuffer.GetFirstUploadMemoryOffset();

	CullingData cullingData{};
	cullingData.commandCount = static_cast<std::uint32_t>(std::size(m_indirectCommands));
	cullingData.modelTypes = static_cast<std::uint32_t>(std::size(m_modelCountOffsets));
	cullingData.xBounds = XBOUNDS;
	cullingData.yBounds = YBOUNDS;
	cullingData.zBounds = ZBOUNDS;

	memcpy(cullingBufferPtr, &cullingData, sizeof(CullingData));

	// Copy modelCount offsets
	for (auto& counterBuffer : m_counterBuffers) {
		std::uint8_t* offsetBufferPtr =
			uploadMemoryStart + counterBuffer.GetFirstUploadMemoryOffset();

		struct {
			std::uint32_t counter;
			std::uint32_t modelCountOffset;
		}sourceCountOffset{ 0u, 0u };
		size_t destOffset = 0u;

		for (auto modelCountOffset : m_modelCountOffsets) {
			sourceCountOffset.modelCountOffset = modelCountOffset;

			memcpy(
				offsetBufferPtr + destOffset, &sourceCountOffset,
				COUNTERBUFFERSTRIDE
			);

			destOffset += COUNTERBUFFERSTRIDE;
		}
	}

	// copy zero to counter buffer
	std::uint8_t* cpuWriteMemoryStart = Terra::Get().Res().CPU().GetMappedCPUPtr();

	std::uint8_t* counterCPUPtr =
		cpuWriteMemoryStart + m_counterResetBuffer.GetFirstMemoryOffset();
	const std::uint32_t zeroValue = 0u;
	memcpy(counterCPUPtr, &zeroValue, sizeof(std::uint32_t));
}

void ComputePipelineIndirectDraw::RecordCopy(VkCommandBuffer transferBuffer) noexcept {
	m_commandBuffer.RecordCopy(transferBuffer);
	m_cullDataBuffer.RecordCopy(transferBuffer);

	for (auto& counterBuffer : m_counterBuffers)
		counterBuffer.RecordCopy(transferBuffer);
}

void ComputePipelineIndirectDraw::ReleaseUploadResources() noexcept {
	m_commandBuffer.CleanUpUploadResource();
	m_cullDataBuffer.CleanUpUploadResource();

	for (auto& counterBuffer : m_counterBuffers)
		counterBuffer.CleanUpUploadResource();

	m_indirectCommands = std::vector<VkDrawIndexedIndirectCommand>();
}

void ComputePipelineIndirectDraw::AcquireOwnerShip(VkCommandBuffer computeCmdBuffer) noexcept {
	m_commandBuffer.AcquireOwnership(
		computeCmdBuffer, m_queueIndices.transfer, m_queueIndices.compute,
		VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	);
	m_cullDataBuffer.AcquireOwnership(
		computeCmdBuffer, m_queueIndices.transfer, m_queueIndices.compute,
		VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
	);
}

void ComputePipelineIndirectDraw::ReleaseOwnership(VkCommandBuffer transferCmdBuffer) noexcept {
	m_commandBuffer.ReleaseOwnerShip(
		transferCmdBuffer, m_queueIndices.transfer, m_queueIndices.compute
	);
	m_cullDataBuffer.ReleaseOwnerShip(
		transferCmdBuffer, m_queueIndices.transfer, m_queueIndices.compute
	);
}

void ComputePipelineIndirectDraw::ResetCounterBuffer(
	VkCommandBuffer computeCmdBuffer, VkDeviceSize frameIndex
) noexcept {
	auto& counterBuffer = m_counterBuffers[frameIndex];

	for (size_t index = 0u; index < std::size(m_modelCountOffsets); ++index) {
		VkBufferCopy bufferInfo{
			.srcOffset = m_counterResetBuffer.GetFirstSubAllocationOffset(),
			.dstOffset = COUNTERBUFFERSTRIDE * index,
			.size = sizeof(std::uint32_t)
		};

		vkCmdCopyBuffer(
			computeCmdBuffer, m_counterResetBuffer.GetResource(), counterBuffer.GetResource(),
			1u, &bufferInfo
		);
	}

	VkBufferBarrier2{}.AddMemoryBarrier(
		BufferBarrierBuilder{}
		.Buffer(
			counterBuffer.GetResource(), counterBuffer.GetSubBufferSize(),
			counterBuffer.GetFirstSubAllocationOffset()
		)
		.AccessMasks(VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
		.StageMasks(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT)
	).RecordBarriers(computeCmdBuffer);
}

std::uint32_t ComputePipelineIndirectDraw::GetCurrentModelCount() const noexcept {
	return m_modelCount;
}

size_t ComputePipelineIndirectDraw::GetCounterCount() const noexcept {
	return std::size(m_modelCountOffsets);
}

VkBuffer ComputePipelineIndirectDraw::GetArgumentBuffer(size_t frameIndex) const noexcept{
	return m_argumentBuffers[frameIndex].GetResource();
}

VkBuffer ComputePipelineIndirectDraw::GetCounterBuffer(size_t frameIndex) const noexcept {
	return m_counterBuffers[frameIndex].GetResource();
}
