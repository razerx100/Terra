#include <unordered_map>
#include <VkModelManager.hpp>
#include <VectorToSharedPtr.hpp>
#include <VkResourceBarriers2.hpp>

namespace Terra
{
// Model Manager VS Individual
void ModelManagerVSIndividual::SetGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = PipelineModelsVSIndividual::GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
}

void ModelManagerVSIndividual::DrawPipeline(
	size_t modelBundleIndex, size_t pipelineLocalIndex, const VKCommandBuffer& graphicsBuffer,
	const MeshManagerVSIndividual& meshManager, VkPipelineLayout pipelineLayout
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const ModelBundleVSIndividual& modelBundle = m_modelBundles[modelBundleIndex];

	// Mesh
	const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	// Model
	modelBundle.DrawPipeline(pipelineLocalIndex, graphicsBuffer, pipelineLayout, meshBundle);
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3,
	std::uint32_t frameCount
) : ModelManager{}, m_argumentInputBuffers{}, m_argumentOutputBuffers{},
	m_modelIndicesBuffers{},
	m_perPipelineBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTC>()
	}, m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
	m_perModelBundleBuffer{ device, memoryManager, frameCount },
	m_perModelBuffer{
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
		m_modelIndicesBuffers.emplace_back(
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

void ModelManagerVSIndirect::ReconfigureModels(
	std::uint32_t bundleIndex, std::uint32_t decreasedModelsPipelineIndex,
	std::uint32_t increasedModelsPipelineIndex
) {
	m_modelBundles[bundleIndex].ReconfigureModels(
		bundleIndex, decreasedModelsPipelineIndex, increasedModelsPipelineIndex,
		m_argumentInputBuffers, m_perPipelineBuffer, m_perModelBuffer,
		m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesBuffers
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
		m_perModelBuffer.Size() / PipelineModelsCSIndirect::GetPerModelStride()
	);

	// ThreadBlockSize is the number of threads in a thread group. If the allocated model count
	// is more than the BlockSize then dispatch more groups. Ex: Threads 64, Model 60 = Group 1
	// Threads 64, Model 65 = Group 2.
	m_dispatchXCount = static_cast<std::uint32_t>(
		std::ceil(m_allocatedModelCount / THREADBLOCKSIZE)
	);
}

std::uint32_t ModelManagerVSIndirect::AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
{
	const size_t bundleIndex  = m_modelBundles.Add(ModelBundleVSIndirect{});

	const auto bundleIndexU32 = static_cast<std::uint32_t>(bundleIndex);

	ModelBundleVSIndirect& localModelBundle = m_modelBundles[bundleIndex];

	localModelBundle.SetModelBundle(std::move(modelBundle));

	localModelBundle.AddNewPipelinesFromBundle(
		bundleIndexU32, m_argumentInputBuffers, m_perPipelineBuffer, m_perModelBuffer,
		m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesBuffers
	);

	UpdateCounterResetValues();

	m_perModelBundleBuffer.ExtendBufferIfNecessaryFor(bundleIndex);

	UpdateAllocatedModelCount();

	return bundleIndexU32;
}

std::shared_ptr<ModelBundle> ModelManagerVSIndirect::RemoveModelBundle(
	std::uint32_t bundleIndex
) noexcept {
	const size_t bundleIndexST = bundleIndex;

	ModelBundleVSIndirect& localModelBundle = m_modelBundles[bundleIndexST];

	std::shared_ptr<ModelBundle> modelBundle = localModelBundle.GetModelBundle();

	localModelBundle.CleanupData(
		m_argumentInputBuffers, m_perPipelineBuffer, m_perModelBuffer,
		m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesBuffers
	);

	m_modelBundles.RemoveElement(bundleIndexST);

	return modelBundle;
}

void ModelManagerVSIndirect::UpdatePipelinePerFrame(
	VkDeviceSize frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
	const MeshManagerVSIndirect& meshManager, bool skipCulling
) const noexcept {
	std::uint8_t* bufferOffsetPtr = m_perModelBundleBuffer.GetInstancePtr(frameIndex);
	constexpr size_t strideSize   = sizeof(PerModelBundleData);

	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const VkDeviceSize bufferOffset       = strideSize * modelBundleIndex;

	const ModelBundleVSIndirect& vsBundle = m_modelBundles[modelBundleIndex];

	const std::uint32_t meshBundleIndex   = vsBundle.GetMeshBundleIndex();

	const VkMeshBundleVS& meshBundle      = meshManager.GetBundle(meshBundleIndex);

	vsBundle.UpdatePipeline(
		pipelineLocalIndex, static_cast<size_t>(frameIndex), meshBundle, skipCulling
	);

	memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);
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
			m_modelIndicesBuffers[index].GetBuffer(), s_modelIndicesVSBindingSlot,
			vsSetLayoutIndex, 0u
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const noexcept {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
	{
		descriptorBuffer.AddBinding(
			s_argumentInputBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_perPipelineBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
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
			s_perModelBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_perModelBundleBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
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
			m_argumentInputBuffers[index].GetBuffer(), s_argumentInputBindingSlot,
			csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_perPipelineBuffer.GetBuffer(), s_perPipelineBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_argumentOutputBuffers[index].GetBuffer(), s_argumenOutputBindingSlot,
			csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_counterBuffers[index].GetBuffer(), s_counterBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_perModelBuffer.GetBuffer(), s_perModelBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesBuffers[index].GetBuffer(), s_modelIndicesVSCSBindingSlot,
			csSetLayoutIndex, 0u
		);

		m_perModelBundleBuffer.SetDescriptorBuffer(
			descriptorBuffer, s_perModelBundleBindingSlot, csSetLayoutIndex
		);
	}
}

void ModelManagerVSIndirect::Dispatch(
	const VKCommandBuffer& computeBuffer,
	const PipelineManager<ComputePipeline_t>& pipelineManager
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

void ModelManagerVSIndirect::DrawPipeline(
	size_t frameIndex, size_t modelBundleIndex, size_t pipelineLocalIndex,
	const VKCommandBuffer& graphicsBuffer, const MeshManagerVSIndirect& meshManager,
	VkPipelineLayout pipelineLayout
) const noexcept {
	if (!m_modelBundles.IsInUse(modelBundleIndex))
		return;

	const ModelBundleVSIndirect& modelBundle = m_modelBundles[modelBundleIndex];

	const VkMeshBundleVS& meshBundle = meshManager.GetBundle(modelBundle.GetMeshBundleIndex());

	modelBundle.DrawPipeline(
		pipelineLocalIndex, frameIndex, graphicsBuffer, pipelineLayout, meshBundle
	);
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
	constexpr std::uint32_t modelConstantSize
		= PipelineModelsMSIndividual::GetConstantBufferSize();
	constexpr std::uint32_t meshConstantSize = VkMeshBundleMS::GetConstantBufferSize();

	layout.AddPushConstantRange(
		VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
		modelConstantSize + meshConstantSize
	);
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

	// Model
	modelBundle.DrawPipeline(pipelineLocalIndex, graphicsBuffer, pipelineLayout, meshBundle);
}
}
