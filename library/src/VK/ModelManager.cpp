#include <unordered_map>
#include <ModelManager.hpp>
#include <VectorToSharedPtr.hpp>
#include <VkResourceBarriers2.hpp>

// Model Manager VS Individual
void ModelManagerVSIndividual::SetGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = PipelineModelsVSIndividual::GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
}

void ModelManagerVSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndividual& meshManager,
	const PipelineManager<Pipeline_t>& pipelineManager
) const noexcept {
	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	const size_t bundleCount        = std::size(m_modelBundles);

	for (size_t index = 0u; index < bundleCount; ++index)
	{
		if (!m_modelBundles.IsInUse(index))
			continue;

		const ModelBundleVSIndividual& modelBundle = m_modelBundles[index];

		// Mesh
		const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

		// Model
		modelBundle.Draw(graphicsBuffer, pipelineLayout, meshBundle, pipelineManager);
	}
}

void ModelManagerVSIndividual::DrawSorted(
	const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndividual& meshManager,
	const PipelineManager<Pipeline_t>& pipelineManager
) noexcept {
	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	const size_t bundleCount        = std::size(m_modelBundles);

	for (size_t index = 0u; index < bundleCount; ++index)
	{
		if (!m_modelBundles.IsInUse(index))
			continue;

		ModelBundleVSIndividual& modelBundle = m_modelBundles[index];
		// Mesh
		const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

		// Model
		modelBundle.DrawSorted(graphicsBuffer, pipelineLayout, meshBundle, pipelineManager);
	}
}

void ModelManagerVSIndividual::DrawPipeline(
	size_t modelBundleIndex, size_t pipelineLocalIndex,
	const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndividual& meshManager,
	VkPipelineLayout pipelineLayout
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const ModelBundleVSIndividual& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	modelBundle.DrawPipeline(pipelineLocalIndex, graphicsBuffer, pipelineLayout, meshBundle);
}

void ModelManagerVSIndividual::DrawPipelineSorted(
	size_t modelBundleIndex, size_t pipelineLocalIndex,
	const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndividual& meshManager,
	VkPipelineLayout pipelineLayout
) noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	ModelBundleVSIndividual& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	modelBundle.DrawPipelineSorted(pipelineLocalIndex, graphicsBuffer, pipelineLayout, meshBundle);
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3,
	std::uint32_t frameCount
) : ModelManager{}, m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_modelIndicesVSBuffers{},
	m_perPipelineDataBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTC>()
	}, m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
	m_perModelBundleBuffer{ device, memoryManager, frameCount },
	m_perModelDataCSBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTC>()
	}, m_queueIndices3{ queueIndices3 }, m_dispatchXCount{ 0u }, m_allocatedModelCount{ 0u },
	m_csPSOIndex{ 0u }
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		// Only getting written and read on the Compute Queue, so should be exclusive resource.
		m_argumentInputBuffers.emplace_back(
			SharedBufferCPU{ device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {} }
		);
		m_argumentOutputBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
		// Doing the resetting on the Compute queue, so CG should be fine.
		m_counterBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
		m_modelIndicesVSBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
	}
}

void ModelManagerVSIndirect::SetGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	constexpr std::uint32_t pushConstantSize = PipelineModelsVSIndirect::GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
}

void ModelManagerVSIndirect::SetComputeConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, pushConstantSize);
}

void ModelManagerVSIndirect::ChangeModelPipeline(
	std::uint32_t bundleIndex, std::uint32_t modelIndexInBundle, std::uint32_t oldPipelineIndex,
	std::uint32_t newPipelineIndex
) {
	m_modelBundles[bundleIndex].MoveModel(
		modelIndexInBundle, bundleIndex, oldPipelineIndex, newPipelineIndex,
		m_argumentInputBuffers, m_perPipelineDataBuffer, m_perModelDataCSBuffer,
		m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesVSBuffers
	);
}

void ModelManagerVSIndirect::UpdateAllocatedModelCount() noexcept
{
	// We can't reduce the amount of allocated model count, as we don't deallocate. We only
	// make the memory available for something else. So, if a bundle from the middle is freed
	// and we decrease the model count, then the last ones will be the ones which won't be rendered.
	// Then again we can't also keep adding the newly added model count, as they might be allocated
	// in some freed memory. We should set the model count to the total allocated model count, that
	// way it won't skip the last ones and also not unnecessarily add extra ones.
	m_allocatedModelCount = static_cast<std::uint32_t>(
		m_perModelDataCSBuffer.Size() / PipelineModelsCSIndirect::GetPerModelStride()
	);

	// ThreadBlockSize is the number of threads in a thread group. If the allocated model count
	// is more than the BlockSize then dispatch more groups. Ex: Threads 64, Model 60 = Group 1
	// Threads 64, Model 65 = Group 2.
	m_dispatchXCount = static_cast<std::uint32_t>(std::ceil(m_allocatedModelCount / THREADBLOCKSIZE));
}

std::uint32_t ModelManagerVSIndirect::AddModelBundle(
	std::shared_ptr<ModelBundle>&& modelBundle, std::vector<std::uint32_t>&& modelBufferIndices
) {
	const size_t bundleIndex  = m_modelBundles.Add(ModelBundleVSIndirect{});
	const auto bundleIndexU32 = static_cast<std::uint32_t>(bundleIndex);

	ModelBundleVSIndirect& localModelBundle = m_modelBundles[bundleIndex];

	localModelBundle.SetModelIndices(std::move(modelBufferIndices));

	_addModelsFromBundle(localModelBundle, *modelBundle, bundleIndexU32);

	UpdateCounterResetValues();

	m_perModelBundleBuffer.ExtendBufferIfNecessaryFor(bundleIndex);

	UpdateAllocatedModelCount();

	localModelBundle.SetModelBundle(std::move(modelBundle));

	return bundleIndexU32;
}

void ModelManagerVSIndirect::_addModelsFromBundle(
	ModelBundleVSIndirect& localModelBundle, const ModelBundle& modelBundle, std::uint32_t modelBundleIndex
) {
	const std::vector<std::shared_ptr<Model>>& models = modelBundle.GetModels();

	const size_t modelCount = std::size(models);

	std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> pipelineModelIndicesMap{};

	for (size_t index = 0u; index < modelCount; ++index)
	{
		const std::shared_ptr<Model>& model      = models[index];

		std::vector<std::uint32_t>& modelIndices = pipelineModelIndicesMap[model->GetPipelineIndex()];

		modelIndices.emplace_back(static_cast<std::uint32_t>(index));
	}

	for (const auto& pipelineModelIndices : pipelineModelIndicesMap)
		localModelBundle.AddModels(
			pipelineModelIndices.first, modelBundleIndex, pipelineModelIndices.second,
			m_argumentInputBuffers, m_perPipelineDataBuffer, m_perModelDataCSBuffer,
			m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesVSBuffers
		);
}

std::vector<std::uint32_t> ModelManagerVSIndirect::RemoveModelBundle(std::uint32_t bundleIndex) noexcept
{
	const size_t bundleIndexST              = bundleIndex;

	ModelBundleVSIndirect& localModelBundle = m_modelBundles[bundleIndexST];

	std::vector<std::uint32_t> modelBufferIndices = localModelBundle.TakeModelBufferIndices();

	localModelBundle.CleanupData(
		m_argumentInputBuffers, m_perPipelineDataBuffer, m_perModelDataCSBuffer,
		m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesVSBuffers
	);

	m_modelBundles.RemoveElement(bundleIndexST);

	return modelBufferIndices;
}

void ModelManagerVSIndirect::UpdatePerFrame(
	VkDeviceSize frameIndex, const MeshManagerVSIndirect& meshManager
) const noexcept {
	std::uint8_t* bufferOffsetPtr = m_perModelBundleBuffer.GetInstancePtr(frameIndex);
	constexpr size_t strideSize   = sizeof(PerModelBundleData);
	VkDeviceSize bufferOffset     = 0u;

	const size_t modelBundleCount = std::size(m_modelBundles);

	for (size_t index = 0u; index < modelBundleCount; ++index)
	{
		if (!m_modelBundles.IsInUse(index))
			continue;

		const ModelBundleVSIndirect& vsBundle = m_modelBundles[index];

		const std::uint32_t meshBundleIndex   = vsBundle.GetMeshBundleIndex();

		const VkMeshBundleVS& meshBundle      = meshManager.GetBundle(meshBundleIndex);

		vsBundle.Update(static_cast<size_t>(frameIndex), meshBundle);

		memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);

		bufferOffset += strideSize;
	}
}

void ModelManagerVSIndirect::UpdatePerFrameSorted(
	VkDeviceSize frameIndex, const MeshManagerVSIndirect& meshManager
) noexcept {
	std::uint8_t* bufferOffsetPtr = m_perModelBundleBuffer.GetInstancePtr(frameIndex);
	constexpr size_t strideSize   = sizeof(PerModelBundleData);
	VkDeviceSize bufferOffset     = 0u;

	const size_t modelBundleCount = std::size(m_modelBundles);

	for (size_t index = 0u; index < modelBundleCount; ++index)
	{
		if (!m_modelBundles.IsInUse(index))
			continue;

		ModelBundleVSIndirect& vsBundle     = m_modelBundles[index];

		const std::uint32_t meshBundleIndex = vsBundle.GetMeshBundleIndex();

		const VkMeshBundleVS& meshBundle    = meshManager.GetBundle(meshBundleIndex);

		vsBundle.UpdateSorted(static_cast<size_t>(frameIndex), meshBundle);

		memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);

		bufferOffset += strideSize;
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
			s_perPipelineBufferBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
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
			s_perModelBundleBufferBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
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
			m_perPipelineDataBuffer.GetBuffer(), s_perPipelineBufferBindingSlot, csSetLayoutIndex, 0u
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

		m_perModelBundleBuffer.SetDescriptorBuffer(
			descriptorBuffer, s_perModelBundleBufferBindingSlot, csSetLayoutIndex
		);
	}
}

void ModelManagerVSIndirect::Dispatch(
	const VKCommandBuffer& computeBuffer, const PipelineManager<ComputePipeline_t>& pipelineManager
) const noexcept {
	VkCommandBuffer cmdBuffer       = computeBuffer.Get();

	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	pipelineManager.BindPipeline(m_csPSOIndex, computeBuffer);

	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		const ConstantData constantData
		{
			.allocatedModelCount = m_allocatedModelCount
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
	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	const size_t bundleCount        = std::size(m_modelBundles);

	for (size_t index = 0u; index < bundleCount; ++index)
	{
		if (!m_modelBundles.IsInUse(index))
			continue;

		const ModelBundleVSIndirect& modelBundle = m_modelBundles[index];

		// Mesh
		const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

		// Model
		modelBundle.Draw(frameIndex, graphicsBuffer, pipelineLayout, meshBundle, pipelineManager);
	}
}

void ModelManagerVSIndirect::DrawPipeline(
	size_t frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
	const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndividual& meshManager,
	VkPipelineLayout pipelineLayout
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const ModelBundleVSIndirect& modelBundle = m_modelBundles[modelBundleIndex];

	const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	modelBundle.DrawPipeline(pipelineLocalIndex, frameIndex, graphicsBuffer, pipelineLayout, meshBundle);
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
void ModelManagerMS::SetGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t modelConstantSize = PipelineModelsMSIndividual::GetConstantBufferSize();
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
	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	const size_t bundleCount        = std::size(m_modelBundles);

	for (size_t index = 0u; index < bundleCount; ++index)
	{
		if (!m_modelBundles.IsInUse(index))
			continue;

		const ModelBundleMSIndividual& modelBundle = m_modelBundles[index];

		const VkMeshBundleMS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

		// Model
		modelBundle.Draw(graphicsBuffer, pipelineLayout, meshBundle, pipelineManager);
	}
}

void ModelManagerMS::DrawSorted(
	const VKCommandBuffer& graphicsBuffer, const MeshManagerMS& meshManager,
	const PipelineManager<Pipeline_t>& pipelineManager
) noexcept {
	VkPipelineLayout pipelineLayout = pipelineManager.GetLayout();

	const size_t bundleCount        = std::size(m_modelBundles);

	for (size_t index = 0u; index < bundleCount; ++index)
	{
		if (!m_modelBundles.IsInUse(index))
			continue;

		ModelBundleMSIndividual& modelBundle = m_modelBundles[index];

		const VkMeshBundleMS& meshBundle     = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

		// Model
		modelBundle.DrawSorted(graphicsBuffer, pipelineLayout, meshBundle, pipelineManager);
	}
}

void ModelManagerMS::DrawPipeline(
	size_t modelBundleIndex, size_t pipelineLocalIndex,
	const VKCommandBuffer& graphicsBuffer, const MeshManagerMS& meshManager,
	VkPipelineLayout pipelineLayout
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const ModelBundleMSIndividual& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const VkMeshBundleMS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	modelBundle.DrawPipeline(pipelineLocalIndex, graphicsBuffer, pipelineLayout, meshBundle);
}

void ModelManagerMS::DrawPipelineSorted(
	size_t modelBundleIndex, size_t pipelineLocalIndex,
	const VKCommandBuffer& graphicsBuffer, const MeshManagerMS& meshManager,
	VkPipelineLayout pipelineLayout
) noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	ModelBundleMSIndividual& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const VkMeshBundleMS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	modelBundle.DrawPipelineSorted(pipelineLocalIndex, graphicsBuffer, pipelineLayout, meshBundle);
}
