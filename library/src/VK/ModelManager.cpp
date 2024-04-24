#include <ModelManager.hpp>

// Model Bundle Vertex Shader Individual
void ModelBundleVertexShaderIndividual::AddModel(
	const VkDrawIndexedIndirectCommand& drawArguments, std::uint32_t modelIndex
) noexcept {
	m_modelDetails.emplace_back(ModelDetails{ modelIndex, drawArguments });
}

void ModelBundleVertexShaderIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelDetail : m_modelDetails)
	{
		constexpr auto pushConstantSize = static_cast<std::uint32_t>(sizeof(std::uint32_t));

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
			pushConstantSize, &modelDetail.modelIndex
		);

		const VkDrawIndexedIndirectCommand& modelArgs = modelDetail.indexedArguments;
		vkCmdDrawIndexed(
			cmdBuffer, modelArgs.indexCount, modelArgs.instanceCount,
			modelArgs.firstIndex, modelArgs.vertexOffset, modelArgs.firstInstance
		);
	}
}

// Model Bundle Mesh Shader
void ModelBundleMeshShader::AddModel(std::uint32_t meshletCount, std::uint32_t modelIndex) noexcept
{
	m_modelDetails.emplace_back(ModelDetails{ modelIndex, meshletCount });
}

void ModelBundleMeshShader::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	using MS = VkDeviceExtension::VkExtMeshShader;
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelDetail : m_modelDetails)
	{
		constexpr auto pushConstantSize = static_cast<std::uint32_t>(sizeof(std::uint32_t));

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_MESH_BIT_EXT, 0u,
			pushConstantSize, &modelDetail.modelIndex
		);

		MS::vkCmdDrawMeshTasksEXT(cmdBuffer, modelDetail.meshletCount, 1u, 1u);
	}
}

// Model Bundle Vertex Shader Indirect
ModelBundleVertexShaderIndirect::ModelBundleVertexShaderIndirect(
	VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount,
	QueueIndices3 queueIndices
) : ModelBundle{}, m_modelCount{ 0u }, m_queueIndices{ queueIndices },
	m_argumentBufferSize{ 0u },
	m_counterBufferSize{ static_cast<VkDeviceSize>(sizeof(std::uint32_t)) },
	m_argumentBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_counterBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_counterResetBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_counterResetData{ std::make_unique<std::uint32_t>(0u) },
	m_frameCount{ frameCount }
{}

void ModelBundleVertexShaderIndirect::CreateBuffers(
	std::uint32_t modelCount, StagingBufferManager& stagingBufferMan
) {
	m_modelCount = modelCount;

	constexpr size_t strideSize = sizeof(ModelBundleComputeShaderIndirect::Argument);
	m_argumentBufferSize        = static_cast<VkDeviceSize>(m_modelCount * strideSize);

	const VkDeviceSize argumentBufferTotalSize = m_argumentBufferSize * m_frameCount;
	const VkDeviceSize counterBufferTotalSize  = m_counterBufferSize * m_frameCount;

	m_argumentBuffer.Create(
		argumentBufferTotalSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		m_queueIndices.ResolveQueueIndices<QueueIndicesCG>()
	);
	m_counterBuffer.Create(
		counterBufferTotalSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		m_queueIndices.ResolveQueueIndices<QueueIndices3>()
	);
	m_counterResetBuffer.Create(
		m_counterBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		{}
	);

	stagingBufferMan.AddBuffer(
		m_counterResetData.get(), m_counterBufferSize, &m_counterResetBuffer, 0u
	);
}

void ModelBundleVertexShaderIndirect::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex,
	std::uint32_t argumentsBindingSlot, std::uint32_t counterBindingSlot
) const noexcept {
	const auto argumentBufferOffset = static_cast<VkDeviceAddress>(frameIndex * m_argumentBufferSize);
	const auto counterBufferOffset  = static_cast<VkDeviceAddress>(frameIndex * m_counterBufferSize);

	descriptorBuffer.AddStorageBufferDescriptor(
		m_argumentBuffer, argumentsBindingSlot, argumentBufferOffset, m_argumentBufferSize
	);
	descriptorBuffer.AddStorageBufferDescriptor(
		m_counterBuffer, counterBindingSlot, counterBufferOffset, m_counterBufferSize
	);
}

void ModelBundleVertexShaderIndirect::Draw(
	const VKCommandBuffer& graphicsBuffer, VkDeviceSize frameIndex
) const noexcept {
	// This stride might change.
	constexpr auto strideSize =
		static_cast<std::uint32_t>(sizeof(ModelBundleComputeShaderIndirect::Argument));

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	vkCmdDrawIndexedIndirectCount(
		cmdBuffer, m_argumentBuffer.Get(), m_argumentBufferSize * frameIndex,
		m_counterBuffer.Get(), m_counterBufferSize * frameIndex, m_modelCount, strideSize
	);
}

void ModelBundleVertexShaderIndirect::ResetCounterBuffer(
	VKCommandBuffer& transferBuffer, VkDeviceSize frameIndex
) const noexcept {
	BufferToBufferCopyBuilder builder{};
	builder.Size(m_counterBufferSize).DstOffset(m_counterBufferSize * frameIndex);

	transferBuffer.Copy(m_counterResetBuffer, m_counterBuffer, builder);
}

void ModelBundleVertexShaderIndirect::CleanupTempData() noexcept
{
	m_counterResetData.reset();
}

// Model Bundle Compute Shader Indirect
ModelBundleComputeShaderIndirect::ModelBundleComputeShaderIndirect(
	VkDevice device, MemoryManager* memoryManager
) : m_argumentInputBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_cullingDataBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_indirectArguments{},
	m_cullingData{
		std::make_unique<CullingData>(
			CullingData{
				.commandCount = 0u,
				.xBounds      = XBOUNDS,
				.yBounds      = YBOUNDS,
				.zBounds      = ZBOUNDS
			}
		)
	}, m_dispatchXCount{ 0u }
{}


void ModelBundleComputeShaderIndirect::AddModel(
	const VkDrawIndexedIndirectCommand& drawArguments, std::uint32_t modelIndex
) noexcept {
	m_indirectArguments.emplace_back(Argument{ drawArguments, modelIndex });
}

void ModelBundleComputeShaderIndirect::CreateBuffers(StagingBufferManager& stagingBufferMan)
{
	constexpr size_t strideSize   = sizeof(ModelBundleComputeShaderIndirect::Argument);
	const auto argumentCount      = static_cast<std::uint32_t>(std::size(m_indirectArguments));
	m_cullingData->commandCount   = argumentCount;

	m_dispatchXCount = static_cast<std::uint32_t>(std::ceil(argumentCount / THREADBLOCKSIZE));

	const auto argumentBufferSize = static_cast<VkDeviceSize>(strideSize * argumentCount);
	const auto cullingDataSize    = static_cast<VkDeviceSize>(sizeof(CullingData));

	m_argumentInputBuffer.Create(
		argumentBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		{}
	);
	m_cullingDataBuffer.Create(
		cullingDataSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		{}
	);

	stagingBufferMan.AddBuffer(
		std::data(m_indirectArguments), argumentBufferSize, &m_argumentInputBuffer, 0u
	);
	stagingBufferMan.AddBuffer(m_cullingData.get(), cullingDataSize, &m_cullingDataBuffer, 0u);
}

void ModelBundleComputeShaderIndirect::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer,
	std::uint32_t argumentInputBindingSlot, std::uint32_t cullingDataBindingSlot
) const noexcept {
	descriptorBuffer.AddStorageBufferDescriptor(m_argumentInputBuffer, argumentInputBindingSlot);
	descriptorBuffer.AddStorageBufferDescriptor(m_cullingDataBuffer, cullingDataBindingSlot);
}

void ModelBundleComputeShaderIndirect::Dispatch(const VKCommandBuffer& computeBuffer) const noexcept
{
	VkCommandBuffer cmdBuffer = computeBuffer.Get();

	vkCmdDispatch(cmdBuffer, m_dispatchXCount, 1u, 1u);
}

void ModelBundleComputeShaderIndirect::CleanupTempData() noexcept
{
	m_indirectArguments = std::vector<Argument>{};
	m_cullingData.reset();
}
