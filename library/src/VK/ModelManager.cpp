#include <ModelManager.hpp>

// Model Bundle Vertex Shader Individual
void ModelBundleVertexShaderIndividual::AddModel(
	const VkDrawIndexedIndirectCommand& drawArguments, std::uint32_t modelIndex
) noexcept {
	m_modelDetails.emplace_back(ModelDetails{ modelIndex, drawArguments });
}

void ModelBundleVertexShaderIndividual::Draw(
	VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) {
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelDetail : m_modelDetails)
	{
		static constexpr auto pushConstantSize = static_cast<std::uint32_t>(sizeof(std::uint32_t) * 2u);

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
	VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) {
	using MS = VkDeviceExtension::VkExtMeshShader;
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelDetail : m_modelDetails)
	{
		static constexpr auto pushConstantSize = static_cast<std::uint32_t>(sizeof(std::uint32_t) * 2u);

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
	m_counterBufferSize{ static_cast<VkDeviceSize>(sizeof(std::uint32_t) * 2u) },
	m_argumentBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_counterBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_frameCount{ frameCount }
{}

void ModelBundleVertexShaderIndirect::AddModels(VkDevice device, std::uint32_t modelCount) noexcept
{
	m_modelCount = modelCount;

	static constexpr size_t strideSize = sizeof(Argument);
	m_argumentBufferSize               = static_cast<VkDeviceSize>(m_modelCount * strideSize);

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

void ModelBundleVertexShaderIndirect::Draw(VKCommandBuffer& graphicsBuffer, VkDeviceSize frameIndex)
{
	// This stride might change.
	static constexpr auto strideSize = static_cast<std::uint32_t>(sizeof(Argument));

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	vkCmdDrawIndexedIndirectCount(
		cmdBuffer, m_argumentBuffer.Get(), m_argumentBufferSize * frameIndex,
		m_counterBuffer.Get(), m_counterBufferSize * frameIndex, m_modelCount, strideSize
	);
}
