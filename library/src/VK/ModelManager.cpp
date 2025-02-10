#include <ModelManager.hpp>
#include <VectorToSharedPtr.hpp>
#include <VkResourceBarriers2.hpp>

// Model Manager VS Individual
ModelManagerVSIndividual::ModelManagerVSIndividual(MemoryManager* memoryManager)
	: ModelManager{ memoryManager }
{}

void ModelManagerVSIndividual::_setGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = ModelBundleVSIndividual::GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
}

void ModelManagerVSIndividual::ConfigureModelBundle(
	ModelBundleVSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle,
	[[maybe_unused]] StagingBufferManager& stagingBufferMan,
	[[maybe_unused]] TemporaryDataBufferGPU& tempBuffer
) const noexcept {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerVSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndividual& meshManager,
	const PipelineManager<Pipeline_t>& pipelineManager
) const noexcept {
	auto previousPSOIndex           = std::numeric_limits<size_t>::max();
	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	for (const ModelBundleVSIndividual& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, pipelineManager, previousPSOIndex);

		// Mesh
		const VkMeshBundleVS& meshBundle = meshManager.GetBundle(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);
		meshBundle.Bind(graphicsBuffer);

		// Model
		modelBundle.Draw(graphicsBuffer, pipelineLayout, meshBundle);
	}
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3,
	std::uint32_t frameCount
) : ModelManager{ memoryManager }, m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_modelIndicesVSBuffers{},
	m_cullingDataBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTC>()
	}, m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
	m_meshBundleIndexBuffer{ device, memoryManager, frameCount },
	m_perModelDataCSBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTC>()
	}, m_queueIndices3{ queueIndices3 }, m_dispatchXCount{ 0u }, m_argumentCount{ 0u },
	m_modelBundlesCS{}, m_oldBufferCopyNecessary{ false }
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		// Only getting written and read on the Compute Queue, so should be exclusive resource.
		m_argumentInputBuffers.emplace_back(
			SharedBufferCPU{ device, m_memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {} }
		);
		m_argumentOutputBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, m_memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
		// Doing the resetting on the Compute queue, so CG should be fine.
		m_counterBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, m_memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
		m_modelIndicesVSBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, m_memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
	}
}

void ModelManagerVSIndirect::_setGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	constexpr std::uint32_t pushConstantSize = ModelBundleVSIndirect::GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
}

void ModelManagerVSIndirect::SetComputeConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, pushConstantSize);
}

void ModelManagerVSIndirect::UpdateDispatchX() noexcept
{
	// ThreadBlockSize is the number of threads in a thread group. If the argumentCount/ModelCount
	// is more than the BlockSize then dispatch more groups. Ex: Threads 64, Model 60 = Group 1
	// Threads 64, Model 65 = Group 2.

	m_dispatchXCount = static_cast<std::uint32_t>(std::ceil(m_argumentCount / THREADBLOCKSIZE));
}

void ModelManagerVSIndirect::ConfigureModelBundle(
	ModelBundleVSIndirect& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle, StagingBufferManager& stagingBufferMan,
	TemporaryDataBufferGPU& tempBuffer
) {
	ModelBundleCSIndirect modelBundleCS{};

	modelBundleCS.SetModelBundle(modelBundle);

	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));

	modelBundleObj.CreateBuffers(m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesVSBuffers);

	UpdateCounterResetValues();

	modelBundleCS.CreateBuffers(
		stagingBufferMan, m_argumentInputBuffers, m_cullingDataBuffer,
		m_perModelDataCSBuffer, modelBundleObj.GetModelIndices(), tempBuffer
	);

	const auto modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

	m_meshBundleIndexBuffer.Add(modelBundleIndexInBuffer);

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	m_argumentCount += modelBundleObj.GetModelCount();

	m_oldBufferCopyNecessary = true;

	UpdateDispatchX();
}

void ModelManagerVSIndirect::ConfigureModelBundleRemove(size_t bundleIndex) noexcept
{
	const ModelBundleVSIndirect& modelBundleVS = m_modelBundles[bundleIndex];

	{
		const std::vector<SharedBufferData>& argumentOutputSharedData
			= modelBundleVS.GetArgumentOutputSharedData();

		for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
			m_argumentOutputBuffers[index].RelinquishMemory(argumentOutputSharedData[index]);

		const std::vector<SharedBufferData>& counterSharedData = modelBundleVS.GetCounterSharedData();

		for (size_t index = 0u; index < std::size(m_counterBuffers); ++index)
			m_counterBuffers[index].RelinquishMemory(counterSharedData[index]);

		const std::vector<SharedBufferData>& modelIndicesSharedData
			= modelBundleVS.GetModelIndicesSharedData();

		for (size_t index = 0u; index < std::size(m_modelIndicesVSBuffers); ++index)
			m_modelIndicesVSBuffers[index].RelinquishMemory(modelIndicesSharedData[index]);
	}

	// The index should be the same as the VS one.
	const ModelBundleCSIndirect& modelBundleCS = m_modelBundlesCS[bundleIndex];

	{
		const std::vector<SharedBufferData>& argumentInputSharedData
			= modelBundleCS.GetArgumentInputSharedData();

		for (size_t index = 0u; index < std::size(m_argumentInputBuffers); ++index)
			m_argumentInputBuffers[index].RelinquishMemory(argumentInputSharedData[index]);

		modelBundleCS.ResetCullingData();

		m_cullingDataBuffer.RelinquishMemory(modelBundleCS.GetCullingSharedData());
		m_perModelDataCSBuffer.RelinquishMemory(modelBundleCS.GetPerModelDataCSSharedData());
	}
}

void ModelManagerVSIndirect::UpdatePerFrame(
	VkDeviceSize frameIndex, const MeshManagerVSIndirect& meshManager
) const noexcept {
	std::uint8_t* bufferOffsetPtr = m_meshBundleIndexBuffer.GetInstancePtr(frameIndex);
	constexpr size_t strideSize   = sizeof(std::uint32_t);
	VkDeviceSize bufferOffset     = 0u;

	// This is necessary because, while the indices should be the same as the CS bundles
	// if we decide to remove a bundle, the bundles afterwards will be shifted. In that
	// case either we can shift the modelBundleIndices to accommodate the bundle changes or
	// write data the modelBundles' old position. I think the second approach is better
	// as the model bundle indices are currently set as GPU only data. While the mesh indices
	// are cpu data which is reset every frame.
	for (const ModelBundleCSIndirect& bundle : m_modelBundlesCS)
	{
		const std::uint32_t meshBundleIndex  = bundle.GetMeshBundleIndex();

		const VkMeshBundleVS& meshBundle     = meshManager.GetBundle(
			static_cast<size_t>(meshBundleIndex)
		);

		bundle.Update(static_cast<size_t>(frameIndex), meshBundle);

		const std::uint32_t modelBundleIndex = bundle.GetModelBundleIndex();

		bufferOffset = strideSize * modelBundleIndex;

		memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex
) const noexcept {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.AddBinding(
			s_modelIndicesVSBindingSlot, vsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
}

void ModelManagerVSIndirect::SetDescriptorBuffersVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex
) const {
	const size_t frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];

		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesVSBuffers[index].GetBuffer(), s_modelIndicesVSBindingSlot, vsSetLayoutIndex, 0u
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const noexcept {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
	{
		descriptorBuffer.AddBinding(
			s_argumentInputBufferBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_cullingDataBufferBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_argumenOutputBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_counterBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_perModelDataCSBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshBundleIndexBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelIndicesVSCSBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBuffersCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const {
	const size_t frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];

		descriptorBuffer.SetStorageBufferDescriptor(
			m_argumentInputBuffers[index].GetBuffer(), s_argumentInputBufferBindingSlot,
			csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_cullingDataBuffer.GetBuffer(), s_cullingDataBufferBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_argumentOutputBuffers[index].GetBuffer(), s_argumenOutputBindingSlot,
			csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_counterBuffers[index].GetBuffer(), s_counterBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_perModelDataCSBuffer.GetBuffer(), s_perModelDataCSBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesVSBuffers[index].GetBuffer(), s_modelIndicesVSCSBindingSlot,
			csSetLayoutIndex, 0u
		);

		m_meshBundleIndexBuffer.SetDescriptorBuffer(
			descriptorBuffer, s_meshBundleIndexBindingSlot, csSetLayoutIndex
		);
	}
}

void ModelManagerVSIndirect::Dispatch(
	const VKCommandBuffer& computeBuffer, const PipelineManager<ComputePipeline_t>& pipelineManager
) const noexcept {
	VkCommandBuffer cmdBuffer       = computeBuffer.Get();

	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	// There should be a single one for now.
	static constexpr size_t computePSOIndex = 0u;

	pipelineManager.BindPipeline(computePSOIndex, computeBuffer);

	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		const ConstantData constantData
		{
			.modelCount = m_argumentCount
		};

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0u,
			pushConstantSize, &constantData
		);
	}

	vkCmdDispatch(cmdBuffer, m_dispatchXCount, 1u, 1u);
}

void ModelManagerVSIndirect::Draw(
	size_t frameIndex, const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndirect& meshManager,
	const PipelineManager<GraphicsPipeline_t>& pipelineManager
) const noexcept {
	auto previousPSOIndex           = std::numeric_limits<size_t>::max();
	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	for (const ModelBundleVSIndirect& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, pipelineManager, previousPSOIndex);

		// Mesh
		const VkMeshBundleVS& meshBundle = meshManager.GetBundle(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);
		meshBundle.Bind(graphicsBuffer);

		// Model
		modelBundle.Draw(frameIndex, graphicsBuffer, pipelineLayout);
	}
}

void ModelManagerVSIndirect::CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_perModelDataCSBuffer.CopyOldBuffer(transferBuffer);
		// I don't think copying is needed for the Output Argument
		// and the counter buffers. As their data will be only
		// needed on the same frame and not afterwards.

		m_oldBufferCopyNecessary = false;
	}
}

void ModelManagerVSIndirect::ResetCounterBuffer(
	const VKCommandBuffer& computeCmdBuffer, size_t frameIndex
) const noexcept {
	const SharedBufferGPUWriteOnly& counterBuffer = m_counterBuffers[frameIndex];

	computeCmdBuffer.CopyWhole(m_counterResetBuffer, counterBuffer.GetBuffer());

	VkBufferBarrier2{}.AddMemoryBarrier(
		BufferBarrierBuilder{}
		.Buffer(counterBuffer.GetBuffer(), counterBuffer.Size())
		.AccessMasks(VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_ACCESS_2_SHADER_READ_BIT)
		.StageMasks(VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
	).RecordBarriers(computeCmdBuffer.Get());
}

void ModelManagerVSIndirect::UpdateCounterResetValues()
{
	if (!std::empty(m_counterBuffers))
	{
		const SharedBufferGPUWriteOnly& counterBuffer = m_counterBuffers.front();

		const VkDeviceSize counterBufferSize = counterBuffer.Size();
		const VkDeviceSize oldCounterSize    = m_counterResetBuffer.BufferSize();

		if (counterBufferSize > oldCounterSize)
		{
			const size_t counterSize = sizeof(std::uint32_t);

			// This should be the source buffer. And should only be accessed from a single type of
			// queue.
			m_counterResetBuffer.Create(counterBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

			constexpr std::uint32_t value = 0u;

			std::uint8_t* bufferStart = m_counterResetBuffer.CPUHandle();

			for (size_t offset = 0u; offset < counterBufferSize; offset += counterSize)
				memcpy(bufferStart + offset, &value, counterSize);
		}
	}
}

// Model Manager MS.
ModelManagerMS::ModelManagerMS(MemoryManager* memoryManager) : ModelManager{ memoryManager } {}

void ModelManagerMS::ConfigureModelBundle(
	ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle,
	[[maybe_unused]] StagingBufferManager& stagingBufferMan,
	[[maybe_unused]] TemporaryDataBufferGPU& tempBuffer
) {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerMS::_setGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t modelConstantSize = ModelBundleMSIndividual::GetConstantBufferSize();
	constexpr std::uint32_t meshConstantSize  = VkMeshBundleMS::GetConstantBufferSize();

	layout.AddPushConstantRange(
		VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
		modelConstantSize + meshConstantSize
	);
}

void ModelManagerMS::Draw(
	const VKCommandBuffer& graphicsBuffer, const MeshManagerMS& meshManager,
	const PipelineManager<Pipeline_t>& pipelineManager
) const noexcept {
	auto previousPSOIndex           = std::numeric_limits<size_t>::max();

	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();
	VkCommandBuffer cmdBuffer       = graphicsBuffer.Get();

	for (const ModelBundleMSIndividual& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, pipelineManager, previousPSOIndex);

		const auto meshBundleIndex = static_cast<size_t>(
			modelBundle.GetMeshBundleIndex()
		);
		const VkMeshBundleMS& meshBundle = meshManager.GetBundle(meshBundleIndex);

		constexpr std::uint32_t constantBufferOffset
			= ModelBundleMSIndividual::GetConstantBufferSize();

		constexpr std::uint32_t constBufferSize = VkMeshBundleMS::GetConstantBufferSize();

		const VkMeshBundleMS::MeshBundleDetailsMS meshBundleDetailsMS
			= meshBundle.GetMeshBundleDetailsMS();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout,
			VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
			constantBufferOffset, constBufferSize, &meshBundleDetailsMS
		);

		// Model
		modelBundle.Draw(graphicsBuffer, pipelineLayout, meshBundle);
	}
}
