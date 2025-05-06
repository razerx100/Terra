#include <VkModelBundle.hpp>

namespace Terra
{
// Pipeline Models Base
VkDrawIndexedIndirectCommand PipelineModelsBase::GetDrawIndexedIndirectCommand(
	const MeshTemporaryDetailsVS& meshDetailsVS
) noexcept {
	const VkDrawIndexedIndirectCommand indirectCommand
	{
		.indexCount    = meshDetailsVS.indexCount,
		.instanceCount = 1u,
		.firstIndex    = meshDetailsVS.indexOffset,
		.vertexOffset  = 0,
		.firstInstance = 0u
	};

	return indirectCommand;
}

// Pipeline Models VS Individual
void PipelineModelsVSIndividual::DrawModel(
	const std::shared_ptr<Model>& model, VkCommandBuffer graphicsCmdBuffer,
	VkPipelineLayout pipelineLayout, const VkMeshBundleVS& meshBundle
) const noexcept {
	if (!model->IsVisible())
		return;

	constexpr std::uint32_t pushConstantSize = GetConstantBufferSize();

	const std::uint32_t indexInBuffer = model->GetModelIndexInBuffer();

	vkCmdPushConstants(
		graphicsCmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
		pushConstantSize, &indexInBuffer
	);

	const MeshTemporaryDetailsVS& meshDetailsVS = meshBundle.GetMeshDetails(model->GetMeshIndex());
	const VkDrawIndexedIndirectCommand meshArgs = GetDrawIndexedIndirectCommand(meshDetailsVS);

	vkCmdDrawIndexed(
		graphicsCmdBuffer, meshArgs.indexCount, meshArgs.instanceCount,
		meshArgs.firstIndex, meshArgs.vertexOffset, meshArgs.firstInstance
	);
}

void PipelineModelsVSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
	const VkMeshBundleVS& meshBundle, const PipelineModelBundle& pipelineBundle,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const std::vector<std::uint32_t>& pipelineModelIndices
		= pipelineBundle.GetModelIndicesInBundle();

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (std::uint32_t modelIndexInBundle : pipelineModelIndices)
		DrawModel(models[modelIndexInBundle], cmdBuffer, pipelineLayout, meshBundle);
}

// Pipeline Models MS Individual
void PipelineModelsMSIndividual::DrawModel(
	const std::shared_ptr<Model>& model, VkCommandBuffer graphicsCmdBuffer,
	VkPipelineLayout pipelineLayout, const VkMeshBundleMS& meshBundle
) const noexcept {
	using MS = VkDeviceExtension::VkExtMeshShader;

	if (!model->IsVisible())
		return;

	constexpr std::uint32_t pushConstantSize    = GetConstantBufferSize();

	constexpr std::uint32_t constBufferOffset   = VkMeshBundleMS::GetConstantBufferSize();

	const MeshTemporaryDetailsMS& meshDetailsMS = meshBundle.GetMeshDetails(
		model->GetMeshIndex()
	);

	const ModelDetails modelConstants
	{
		.meshDetails = MeshDetails
			{
				.meshletCount  = meshDetailsMS.meshletCount,
				.meshletOffset = meshDetailsMS.meshletOffset,
				.indexOffset   = meshDetailsMS.indexOffset,
				.primOffset    = meshDetailsMS.primitiveOffset,
				.vertexOffset  = meshDetailsMS.vertexOffset
			},
		.modelBufferIndex = model->GetModelIndexInBuffer()
	};

	vkCmdPushConstants(
		graphicsCmdBuffer, pipelineLayout,
		VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
		constBufferOffset, pushConstantSize, &modelConstants
	);

	// If we have a Task shader, this will launch a Task global workGroup. We would want
	// each Task shader invocation to process a meshlet and launch the necessary
	// Mesh Shader workGroups. On Nvdia we can have a maximum of 32 invocations active
	// in a subGroup and 64 on AMD. So, a workGroup will be able to work on 32/64
	// meshlets concurrently.
	const std::uint32_t taskGroupCount = DivRoundUp(
		meshDetailsMS.meshletCount, s_taskInvocationCount
	);

	MS::vkCmdDrawMeshTasksEXT(graphicsCmdBuffer, taskGroupCount, 1u, 1u);
	// It might be worth checking if we are reaching the Group Count Limit and if needed
	// launch more Groups. Could achieve that by passing a GroupLaunch index.
}

void PipelineModelsMSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
	const VkMeshBundleMS& meshBundle, const PipelineModelBundle& pipelineBundle,
	const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const std::vector<std::uint32_t>& pipelineModelIndices
		= pipelineBundle.GetModelIndicesInBundle();

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (std::uint32_t modelIndexInBundle : pipelineModelIndices)
		DrawModel(models[modelIndexInBundle], cmdBuffer, pipelineLayout, meshBundle);
}

// Pipeline Models CS Indirect
PipelineModelsCSIndirect::PipelineModelsCSIndirect()
	: PipelineModelsBase{}, m_perPipelineSharedData{ nullptr, 0u, 0u },
	m_perModelSharedData{ nullptr, 0u, 0u }, m_argumentInputSharedData{}
{}

void PipelineModelsCSIndirect::ResetCullingData() const noexcept
{
	// Before destroying an object the model count needs to be set to 0,
	// so the models aren't processed in the compute shader anymore.
	if (m_perPipelineSharedData.bufferData)
	{
		std::uint32_t modelCount        = 0u;
		Buffer const* perPipelineBuffer = m_perPipelineSharedData.bufferData;

		memcpy(
			perPipelineBuffer->CPUHandle() + m_perPipelineSharedData.offset,
			&modelCount, sizeof(std::uint32_t)
		);
	}
}

size_t PipelineModelsCSIndirect::GetAddableModelCount(
	const std::vector<PipelineModelBundle>& pipelineBundles
) const noexcept {
	constexpr size_t perModelStride = sizeof(PerModelData);

	const size_t currentModelCount  = GetModelCount(pipelineBundles);

	const auto currentPerModelBufferSize = static_cast<size_t>(m_perModelSharedData.size);

	const size_t allocatedModelCount     = currentPerModelBufferSize / perModelStride;

	return allocatedModelCount > currentModelCount ? allocatedModelCount - currentModelCount : 0u;
}

size_t PipelineModelsCSIndirect::GetNewModelCount(
	const std::vector<PipelineModelBundle>& pipelineBundles
) const noexcept {
	constexpr size_t perModelStride = sizeof(PerModelData);

	const size_t currentModelCount  = GetModelCount(pipelineBundles);

	const auto currentPerModelBufferSize = static_cast<size_t>(m_perModelSharedData.size);

	const size_t allocatedModelCount     = currentPerModelBufferSize / perModelStride;

	return allocatedModelCount < currentModelCount ?  currentModelCount - allocatedModelCount : 0u;
}

void PipelineModelsCSIndirect::UpdateNonPerFrameData(
	std::uint32_t modelBundleIndex, const std::vector<PipelineModelBundle>& pipelineBundles
) noexcept {
	constexpr size_t argumentStrideSize = sizeof(VkDrawIndexedIndirectCommand);
	constexpr size_t perModelStride     = sizeof(PerModelData);
	constexpr size_t perPipelineStride  = sizeof(PerPipelineData);

	const size_t modelCount = GetModelCount(pipelineBundles);

	if (!modelCount)
		return;

	// Per Pipeline Data.
	{
		PerPipelineData perPipelineData
		{
			.modelCount       = static_cast<std::uint32_t>(modelCount),
			.modelOffset      = 0u,
			.modelBundleIndex = modelBundleIndex
		};

		if (!std::empty(m_argumentInputSharedData))
		{
			const SharedBufferData& sharedBufferData = m_argumentInputSharedData.front();

			perPipelineData.modelOffset = static_cast<std::uint32_t>(
				sharedBufferData.offset / argumentStrideSize
			);
		}

		Buffer const* perPipelineBuffer = m_perPipelineSharedData.bufferData;

		memcpy(
			perPipelineBuffer->CPUHandle() + m_perPipelineSharedData.offset,
			&perPipelineData, perPipelineStride
		);
	}

	// Per Model Data
	{
		const auto pipelineIndex = static_cast<std::uint32_t>(
			m_perPipelineSharedData.offset / perPipelineStride
		);

		std::uint8_t* bufferStart = m_perModelSharedData.bufferData->CPUHandle();
		auto offset               = static_cast<size_t>(m_perModelSharedData.offset);

		for (size_t index = 0u; index < modelCount; ++index)
		{
			PerModelData perModelData
			{
				.pipelineIndex = pipelineIndex,
				// The model index and flags will be updated every frame.
				.modelIndex    = std::numeric_limits<std::uint32_t>::max(),
				.modelFlags    = static_cast<std::uint32_t>(ModelFlag::Visibility)
			};

			memcpy(bufferStart + offset, &perModelData, perModelStride);

			offset += perModelStride;
		}
	}
}

void PipelineModelsCSIndirect::AllocateBuffers(
	const std::vector<PipelineModelBundle>& pipelineBundles,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
) {
	const size_t modelCount = GetModelCount(pipelineBundles);

	if (!modelCount)
		return;

	constexpr size_t argumentStrideSize = sizeof(VkDrawIndexedIndirectCommand);
	constexpr size_t perModelStride     = sizeof(PerModelData);
	constexpr auto perPipelineDataSize  = static_cast<VkDeviceSize>(sizeof(PerPipelineData));

	const auto argumentBufferSize = static_cast<VkDeviceSize>(argumentStrideSize * modelCount);
	const auto perModelDataSize   = static_cast<VkDeviceSize>(perModelStride * modelCount);

	// Input Arguments
	const size_t argumentInputBufferCount = std::size(argumentInputSharedBuffers);

	if (std::empty(m_argumentInputSharedData))
		m_argumentInputSharedData.resize(
			argumentInputBufferCount, SharedBufferData{ nullptr, 0u, 0u }
		);

	for (size_t index = 0u; index < argumentInputBufferCount; ++index)
	{
		SharedBufferData& argumentInputSharedData  = m_argumentInputSharedData[index];
		SharedBufferCPU& argumentInputSharedBuffer = argumentInputSharedBuffers[index];

		if (argumentInputSharedData.bufferData)
			argumentInputSharedBuffer.RelinquishMemory(argumentInputSharedData);

		argumentInputSharedData = argumentInputSharedBuffer.AllocateAndGetSharedData(
			argumentBufferSize, true
		);
	}

	// Per Pipeline Data.
	{
		if (m_perPipelineSharedData.bufferData)
			perPipelineSharedBuffer.RelinquishMemory(m_perPipelineSharedData);

		m_perPipelineSharedData = perPipelineSharedBuffer.AllocateAndGetSharedData(
			perPipelineDataSize, true
		);
	}

	// Per Model Data
	{
		if (m_perModelSharedData.bufferData)
			perModelSharedBuffer.RelinquishMemory(m_perModelSharedData);

		m_perModelSharedData = perModelSharedBuffer.AllocateAndGetSharedData(
			perModelDataSize, true
		);
	}
}

void PipelineModelsCSIndirect::Update(
	size_t frameIndex, const VkMeshBundleVS& meshBundle, bool skipCulling,
	const PipelineModelBundle& pipelineBundle, const std::vector<std::shared_ptr<Model>>& models
) const noexcept {
	const std::vector<std::uint32_t>& pipelineModelIndices
		= pipelineBundle.GetModelIndicesInBundle();

	const size_t modelCount = std::size(pipelineModelIndices);

	if (!modelCount)
		return;

	const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[frameIndex];

	std::uint8_t* argumentInputStart  = argumentInputSharedData.bufferData->CPUHandle();
	std::uint8_t* perModelBufferStart = m_perModelSharedData.bufferData->CPUHandle();

	constexpr size_t argumentStride = sizeof(VkDrawIndexedIndirectCommand);
	auto argumentOffset             = static_cast<size_t>(argumentInputSharedData.offset);

	constexpr size_t perModelStride = sizeof(PerModelData);
	auto perModelOffset             = static_cast<size_t>(m_perModelSharedData.offset);
	constexpr auto modelFlagsOffset = offsetof(PerModelData, modelFlags);
	constexpr auto modelIndexOffset = offsetof(PerModelData, modelIndex);

	std::uint32_t skipCullingFlag
		= skipCulling ? static_cast<std::uint32_t>(ModelFlag::SkipCulling) : 0u;

	for (std::uint32_t modelIndexInBundle : pipelineModelIndices)
	{
		const std::shared_ptr<Model>& model = models[modelIndexInBundle];

		const MeshTemporaryDetailsVS& meshDetailsVS
			= meshBundle.GetMeshDetails(model->GetMeshIndex());
		const VkDrawIndexedIndirectCommand meshArgs
			= PipelineModelsBase::GetDrawIndexedIndirectCommand(meshDetailsVS);

		memcpy(argumentInputStart + argumentOffset, &meshArgs, argumentStride);

		argumentOffset += argumentStride;

		// Model Flags
		std::uint32_t modelFlags = skipCullingFlag;

		if (model->IsVisible())
			modelFlags |= static_cast<std::uint32_t>(ModelFlag::Visibility);

		memcpy(
			perModelBufferStart + perModelOffset + modelFlagsOffset,
			&modelFlags, sizeof(std::uint32_t)
		);

		// Model Index
		const std::uint32_t indexInBuffer = model->GetModelIndexInBuffer();

		memcpy(
			perModelBufferStart + perModelOffset + modelIndexOffset,
			&indexInBuffer, sizeof(std::uint32_t)
		);

		perModelOffset += perModelStride;
	}
}

void PipelineModelsCSIndirect::RelinquishMemory(
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelSharedBuffer
) noexcept {
	// Input Buffers
	// The argument input shared data container might be empty due to no models being added.
	// So, have to use its size to free instead of the Shared Buffer's.
	for (size_t index = 0u; index < std::size(m_argumentInputSharedData); ++index)
	{
		SharedBufferCPU& argumentSharedBuffer = argumentInputSharedBuffers[index];
		SharedBufferData& argumentSharedData  = m_argumentInputSharedData[index];

		if (argumentSharedData.bufferData)
		{
			argumentSharedBuffer.RelinquishMemory(argumentSharedData);

			argumentSharedData = SharedBufferData{ nullptr, 0u, 0u };
		}
	}

	// Per Pipeline Buffer
	if (m_perPipelineSharedData.bufferData)
	{
		perPipelineSharedBuffer.RelinquishMemory(m_perPipelineSharedData);

		m_perPipelineSharedData = SharedBufferData{ nullptr, 0u, 0u };
	}

	// Per Model Buffer
	if (m_perModelSharedData.bufferData)
	{
		perModelSharedBuffer.RelinquishMemory(m_perModelSharedData);

		m_perModelSharedData = SharedBufferData{ nullptr, 0u, 0u };
	}
}

// Pipeline Models VS Indirect
PipelineModelsVSIndirect::PipelineModelsVSIndirect()
	:  m_argumentOutputSharedData{},
	m_counterSharedData{}, m_modelIndicesSharedData{}, m_modelCount{ 0u }, m_modelOffset{ 0u }
{}

void PipelineModelsVSIndirect::AllocateBuffers(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) {
	constexpr size_t argStrideSize      = sizeof(VkDrawIndexedIndirectCommand);
	constexpr size_t indexStrideSize    = sizeof(std::uint32_t);
	const auto argumentOutputBufferSize = static_cast<VkDeviceSize>(m_modelCount * argStrideSize);
	const auto modelIndiceBufferSize = static_cast<VkDeviceSize>(m_modelCount * indexStrideSize);

	if (!m_modelCount)
		return;

	// Argument Output
	{
		const size_t argumentOutputBufferCount = std::size(argumentOutputSharedBuffers);

		if (std::empty(m_argumentOutputSharedData))
			m_argumentOutputSharedData.resize(
				argumentOutputBufferCount, SharedBufferData{ nullptr, 0u, 0u }
			);

		for (size_t index = 0u; index < argumentOutputBufferCount; ++index)
		{
			SharedBufferData& argumentOutputSharedData
				= m_argumentOutputSharedData[index];
			SharedBufferGPUWriteOnly& argumentOutputSharedBuffer
				= argumentOutputSharedBuffers[index];

			if (argumentOutputSharedData.bufferData)
				argumentOutputSharedBuffer.RelinquishMemory(argumentOutputSharedData);

			argumentOutputSharedData = argumentOutputSharedBuffer.AllocateAndGetSharedData(
				argumentOutputBufferSize
			);

			// The offset on each sharedBuffer should be the same. But still need to keep track of
			// each of them because we will need the Buffer object to draw.
			m_modelOffset
				= static_cast<std::uint32_t>(argumentOutputSharedData.offset / argStrideSize);
		}
	}

	// Counter Buffer
	{
		const size_t counterBufferCount = std::size(counterSharedBuffers);

		if (std::empty(m_counterSharedData))
			m_counterSharedData.resize(
				counterBufferCount, SharedBufferData{ nullptr, 0u, 0u }
			);

		for (size_t index = 0u; index < counterBufferCount; ++index)
		{
			SharedBufferData& counterSharedData           = m_counterSharedData[index];
			SharedBufferGPUWriteOnly& counterSharedBuffer = counterSharedBuffers[index];

			if (counterSharedData.bufferData)
				counterSharedBuffer.RelinquishMemory(counterSharedData);

			counterSharedData = counterSharedBuffer.AllocateAndGetSharedData(
				s_counterBufferSize
			);
		}
	}

	// Model Indices
	{
		const size_t modelIndicesBufferCount = std::size(modelIndicesSharedBuffers);

		if (std::empty(m_modelIndicesSharedData))
			m_modelIndicesSharedData.resize(
				modelIndicesBufferCount, SharedBufferData{ nullptr, 0u, 0u }
			);

		for (size_t index = 0u; index < modelIndicesBufferCount; ++index)
		{
			SharedBufferData& modelIndicesSharedData           = m_modelIndicesSharedData[index];
			SharedBufferGPUWriteOnly& modelIndicesSharedBuffer = modelIndicesSharedBuffers[index];

			if (modelIndicesSharedData.bufferData)
				modelIndicesSharedBuffer.RelinquishMemory(modelIndicesSharedData);

			modelIndicesSharedData = modelIndicesSharedBuffer.AllocateAndGetSharedData(
				modelIndiceBufferSize
			);
		}
	}
}

void PipelineModelsVSIndirect::Draw(
	size_t frameIndex, const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	constexpr auto strideSize = static_cast<std::uint32_t>(sizeof(VkDrawIndexedIndirectCommand));

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	if (!m_modelCount)
		return;

	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
			pushConstantSize, &m_modelOffset
		);
	}

	const SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[frameIndex];
	const SharedBufferData& counterSharedData        = m_counterSharedData[frameIndex];

	vkCmdDrawIndexedIndirectCount(
		cmdBuffer,
		argumentOutputSharedData.bufferData->Get(), argumentOutputSharedData.offset,
		counterSharedData.bufferData->Get(), counterSharedData.offset,
		m_modelCount, strideSize
	);
}

void PipelineModelsVSIndirect::RelinquishMemory(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) noexcept {
	// The shared data containers might be empty due to no models being added.
	// So, have to use their size to free instead of the Shared Buffers'.

	// Output Buffers
	for (size_t index = 0u; index < std::size(m_argumentOutputSharedData); ++index)
	{
		SharedBufferGPUWriteOnly& argumentSharedBuffer = argumentOutputSharedBuffers[index];
		SharedBufferData& argumentSharedData           = m_argumentOutputSharedData[index];

		if (argumentSharedData.bufferData)
		{
			argumentSharedBuffer.RelinquishMemory(argumentSharedData);

			argumentSharedData = SharedBufferData{ nullptr, 0u, 0u };
		}
	}

	// Counter Shared Buffer
	for (size_t index = 0u; index < std::size(m_counterSharedData); ++index)
	{
		SharedBufferGPUWriteOnly& counterSharedBuffer = counterSharedBuffers[index];
		SharedBufferData& counterSharedData           = m_counterSharedData[index];

		if (counterSharedData.bufferData)
		{
			counterSharedBuffer.RelinquishMemory(counterSharedData);

			counterSharedData = SharedBufferData{ nullptr, 0u, 0u };
		}
	}

	// Model Indices Buffer
	for (size_t index = 0u; index < std::size(m_modelIndicesSharedData); ++index)
	{
		SharedBufferGPUWriteOnly& modelIndicesSharedBuffer = modelIndicesSharedBuffers[index];
		SharedBufferData& modelIndicesSharedData           = m_modelIndicesSharedData[index];

		if (modelIndicesSharedData.bufferData)
		{
			modelIndicesSharedBuffer.RelinquishMemory(modelIndicesSharedData);

			modelIndicesSharedData = SharedBufferData{ nullptr, 0u, 0u };
		}
	}
}

// Model Bundle VS Individual
void ModelBundleVSIndividual::DrawPipeline(
	size_t pipelineLocalIndex, const VKCommandBuffer& graphicsBuffer,
	VkPipelineLayout pipelineLayout, const VkMeshBundleVS& meshBundle
) const noexcept {
	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	meshBundle.Bind(graphicsBuffer);

	const auto& models = m_modelBundle->GetModels();

	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	const PipelineModelsVSIndividual& vkPipeline = m_pipelines[pipelineLocalIndex];

	vkPipeline.Draw(
		graphicsBuffer, pipelineLayout, meshBundle, pipelines[pipelineLocalIndex], models
	);
}

// Model Bundle MS Individual
void ModelBundleMSIndividual::SetMeshBundleConstants(
	VkCommandBuffer graphicsBuffer, VkPipelineLayout pipelineLayout,
	const VkMeshBundleMS& meshBundle
) noexcept {
	constexpr std::uint32_t constBufferSize = VkMeshBundleMS::GetConstantBufferSize();

	const VkMeshBundleMS::MeshBundleDetailsMS meshBundleDetailsMS
		= meshBundle.GetMeshBundleDetailsMS();

	vkCmdPushConstants(
		graphicsBuffer, pipelineLayout,
		VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
		0u, constBufferSize, &meshBundleDetailsMS
	);
}

void ModelBundleMSIndividual::DrawPipeline(
	size_t pipelineLocalIndex, const VKCommandBuffer& graphicsBuffer,
	VkPipelineLayout pipelineLayout, const VkMeshBundleMS& meshBundle
) const noexcept {
	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	SetMeshBundleConstants(graphicsBuffer.Get(), pipelineLayout, meshBundle);

	const auto& models = m_modelBundle->GetModels();

	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	const PipelineModelsMSIndividual& vkPipeline = m_pipelines[pipelineLocalIndex];

	vkPipeline.Draw(
		graphicsBuffer, pipelineLayout, meshBundle, pipelines[pipelineLocalIndex], models
	);
}

// Model Bundle VS Indirect
void ModelBundleVSIndirect::AddNewPipelinesFromBundle(
	std::uint32_t modelBundleIndex, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) {
	const size_t pipelinesInBundle    = m_modelBundle->GetPipelineCount();
	const size_t currentPipelineCount = std::size(m_pipelines);

	for (size_t index = currentPipelineCount; index < pipelinesInBundle; ++index)
	{
		const size_t pipelineLocalIndex = _addPipeline(static_cast<std::uint32_t>(index));

		// Since the cs pipeline is a Reusable vector, it can reuse old pipelines
		// in that case we don't need to create a new vs pipeline.
		if (pipelineLocalIndex >= std::size(m_vsPipelines))
			m_vsPipelines.emplace_back(PipelineModelsVSIndirect{});

		SetupPipelineBuffers(
			static_cast<std::uint32_t>(pipelineLocalIndex), modelBundleIndex,
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
			argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
		);
	}
}

void ModelBundleVSIndirect::CleanupData(
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) noexcept {
	const size_t pipelineCount = std::size(m_pipelines);

	for (size_t index = 0u; index < pipelineCount; ++index)
		RemovePipeline(
			index, argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
			argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
		);

	operator=(ModelBundleVSIndirect{});
}

void ModelBundleVSIndirect::RemovePipeline(
	size_t pipelineLocalIndex, std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) noexcept {
	// CS
	PipelineModelsCSIndirect& csPipeline = m_pipelines[pipelineLocalIndex];

	csPipeline.ResetCullingData();

	csPipeline.RelinquishMemory(
		argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
	);
	// The cleanup will be done in _removePipeline

	// VS
	PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[pipelineLocalIndex];

	vsPipeline.RelinquishMemory(
		argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
	);
	vsPipeline.CleanupData();

	_removePipeline(pipelineLocalIndex);
}

size_t ModelBundleVSIndirect::GetLocalPipelineIndex(std::uint32_t pipelineIndex)
{
	auto pipelineLocalIndex = std::numeric_limits<size_t>::max();

	std::optional<size_t> oPipelineLocalIndex = m_modelBundle->FindLocalPipelineIndex(
		pipelineIndex
	);

	assert(oPipelineLocalIndex && "Local pipeline doesn't exist.");

	if (oPipelineLocalIndex)
		pipelineLocalIndex = oPipelineLocalIndex.value();

	return pipelineLocalIndex;
}

void ModelBundleVSIndirect::ReconfigureModels(
	std::uint32_t modelBundleIndex, std::uint32_t decreasedModelsPipelineIndex,
	std::uint32_t increasedModelsPipelineIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) {
	auto decreasedPipelineLocalIndex = std::numeric_limits<size_t>::max();
	auto increasedPipelineLocalIndex = std::numeric_limits<std::uint32_t>::max();

	// Find the pipelines
	const size_t pipelineCount = std::size(m_pipelines);

	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	for (size_t index = 0u; index < pipelineCount; ++index)
	{
		const std::uint32_t currentPipelineIndex = pipelines[index].GetPipelineIndex();

		if (currentPipelineIndex == decreasedModelsPipelineIndex)
			decreasedPipelineLocalIndex = index;

		if (currentPipelineIndex == increasedModelsPipelineIndex)
			increasedPipelineLocalIndex = static_cast<std::uint32_t>(index);

		const bool bothFound
			= decreasedPipelineLocalIndex != std::numeric_limits<size_t>::max() &&
			increasedPipelineLocalIndex != std::numeric_limits<std::uint32_t>::max();

		if (bothFound)
			break;
	}

	// Need to update the model count on the decreased pipeline. So, the Compute Shader
	// doesn't process the moved model and the Vertex shader doesn't draw it.
	PipelineModelsCSIndirect& csPipeline = m_pipelines[decreasedPipelineLocalIndex];

	csPipeline.UpdateNonPerFrameData(modelBundleIndex, pipelines);

	m_vsPipelines[decreasedPipelineLocalIndex].SetModelCount(csPipeline.GetModelCount(pipelines));

	SetupPipelineBuffers(
		increasedPipelineLocalIndex, modelBundleIndex, argumentInputSharedBuffers,
		perPipelineSharedBuffer, perModelDataCSBuffer, argumentOutputSharedBuffers,
		counterSharedBuffers, modelIndicesSharedBuffers
	);
}

size_t ModelBundleVSIndirect::FindAddableStartIndex(
	size_t pipelineLocalIndex, size_t modelCount
) const noexcept {
	auto addableStartIndex   = std::numeric_limits<size_t>::max();

	size_t totalAddableModel = 0u;

	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	for (size_t index = pipelineLocalIndex; index > 0u; --index)
	{
		const PipelineModelsCSIndirect& pipeline = m_pipelines[index];

		const size_t currentAddableModel = pipeline.GetAddableModelCount(pipelines);

		if (currentAddableModel)
		{
			totalAddableModel += currentAddableModel;

			if (totalAddableModel >= modelCount)
			{
				addableStartIndex = index;

				break;
			}
		}
	}

	if (addableStartIndex == std::numeric_limits<size_t>::max())
	{
		const size_t index = 0u;

		const PipelineModelsCSIndirect& pipeline = m_pipelines[index];

		const size_t currentAddableModel = pipeline.GetAddableModelCount(pipelines);

		if (currentAddableModel)
		{
			totalAddableModel += currentAddableModel;

			if (totalAddableModel >= modelCount)
				addableStartIndex = index;
		}
	}

	return addableStartIndex;
}

void ModelBundleVSIndirect::ResizePreviousPipelines(
	size_t addableStartIndex, size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) {
	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	for (size_t index = addableStartIndex; index < pipelineLocalIndex; ++index)
	{
		PipelineModelsCSIndirect& csPipeline = m_pipelines[index];
		PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[index];

		csPipeline.AllocateBuffers(
			pipelines, argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		csPipeline.UpdateNonPerFrameData(modelBundleIndex, pipelines);

		vsPipeline.SetModelCount(csPipeline.GetModelCount(pipelines));

		vsPipeline.AllocateBuffers(
			argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
		);
	}
}

void ModelBundleVSIndirect::RecreateFollowingPipelines(
	size_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) {
	const size_t pipelineCount = std::size(m_pipelines);

	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	for (size_t index = pipelineLocalIndex; index < pipelineCount; ++index)
	{
		PipelineModelsCSIndirect& csPipeline = m_pipelines[index];
		PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[index];

		// CS
		// Must free the memory first, otherwise the buffers of the current pipeline
		// will be created at the end.
		csPipeline.RelinquishMemory(
			argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		csPipeline.AllocateBuffers(
			pipelines, argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		csPipeline.UpdateNonPerFrameData(modelBundleIndex, pipelines);

		// VS
		vsPipeline.RelinquishMemory(
			argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
		);

		vsPipeline.SetModelCount(csPipeline.GetModelCount(pipelines));

		vsPipeline.AllocateBuffers(
			argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
		);
	}
}

void ModelBundleVSIndirect::SetupPipelineBuffers(
	std::uint32_t pipelineLocalIndex, std::uint32_t modelBundleIndex,
	std::vector<SharedBufferCPU>& argumentInputSharedBuffers,
	SharedBufferCPU& perPipelineSharedBuffer, SharedBufferCPU& perModelDataCSBuffer,
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers
) {
	PipelineModelsCSIndirect& pipeline = m_pipelines[pipelineLocalIndex];

	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	const auto pipelineCount = static_cast<std::uint32_t>(std::size(m_pipelines));

	// We don't have to mess with any other pipelines if this is the last pipeline.
	if (pipelineLocalIndex + 1u == pipelineCount)
	{
		pipeline.AllocateBuffers(
			pipelines, argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer
		);

		pipeline.UpdateNonPerFrameData(modelBundleIndex, pipelines);

		PipelineModelsVSIndirect& vsPipeline = m_vsPipelines[pipelineLocalIndex];

		vsPipeline.SetModelCount(pipeline.GetModelCount(pipelines));

		vsPipeline.AllocateBuffers(
			argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
		);
	}
	else
	{
		const size_t addableStartIndex = FindAddableStartIndex(
			pipelineLocalIndex, pipeline.GetNewModelCount(pipelines)
		);

		// If we have enough free space in the previous pipelines, we should be able to
		// resize them and fit this one. This will be useful for moving and it wouldn't
		// require any actual new buffer allocation.
		if (addableStartIndex != std::numeric_limits<size_t>::max())
			ResizePreviousPipelines(
				addableStartIndex, pipelineLocalIndex, modelBundleIndex,
				argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
				argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
			);
		// Otherwise we will have to increase the buffer size and also recreate all the
		// buffers of the following pipelines.
		else
			RecreateFollowingPipelines(
				pipelineLocalIndex, modelBundleIndex,
				argumentInputSharedBuffers, perPipelineSharedBuffer, perModelDataCSBuffer,
				argumentOutputSharedBuffers, counterSharedBuffers, modelIndicesSharedBuffers
			);
	}
}

void ModelBundleVSIndirect::UpdatePipeline(
	size_t pipelineLocalIndex, size_t frameIndex, const VkMeshBundleVS& meshBundle,
	bool skipCulling
) const noexcept {
	const auto& models = m_modelBundle->GetModels();

	const std::vector<PipelineModelBundle>& pipelines = m_modelBundle->GetPipelineBundles();

	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	const PipelineModelsCSIndirect& vkPipeline = m_pipelines[pipelineLocalIndex];

	vkPipeline.Update(frameIndex, meshBundle, skipCulling, pipelines[pipelineLocalIndex], models);
}

void ModelBundleVSIndirect::DrawPipeline(
	size_t pipelineLocalIndex, size_t frameIndex, const VKCommandBuffer& graphicsBuffer,
	VkPipelineLayout pipelineLayout, const VkMeshBundleVS& meshBundle
) const noexcept {
	if (!m_pipelines.IsInUse(pipelineLocalIndex))
		return;

	meshBundle.Bind(graphicsBuffer);

	const PipelineModelsVSIndirect& pipeline = m_vsPipelines[pipelineLocalIndex];

	pipeline.Draw(frameIndex, graphicsBuffer, pipelineLayout);
}
}
