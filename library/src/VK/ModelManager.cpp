#include <ranges>
#include <algorithm>

#include <ModelManager.hpp>

// Model Bundle
VkDrawIndexedIndirectCommand ModelBundle::GetDrawIndexedIndirectCommand(
	const std::shared_ptr<ModelVS>& model
) noexcept {
	const MeshDetailsVS& meshDetails = model->GetMeshDetailsVS();

	const VkDrawIndexedIndirectCommand indirectCommand{
		.indexCount    = meshDetails.indexCount,
		.instanceCount = 1u,
		.firstIndex    = meshDetails.indexOffset,
		.vertexOffset  = 0u,
		.firstInstance = 0u
	};

	return indirectCommand;
}

// Model Bundle Vertex Shader Individual
void ModelBundleVertexShaderIndividual::AddMeshDetails(
	const std::shared_ptr<ModelVS>& model, std::uint32_t modelBufferIndex
) noexcept {
	m_meshDetails.emplace_back(
		MeshDetails{
			.modelBufferIndex = modelBufferIndex,
			.indexedArguments = GetDrawIndexedIndirectCommand(model)
		}
	);
}

void ModelBundleVertexShaderIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& meshDetail : m_meshDetails)
	{
		constexpr auto pushConstantSize = static_cast<std::uint32_t>(sizeof(std::uint32_t));

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
			pushConstantSize, &meshDetail.modelBufferIndex
		);

		const VkDrawIndexedIndirectCommand& meshArgs = meshDetail.indexedArguments;
		vkCmdDrawIndexed(
			cmdBuffer, meshArgs.indexCount, meshArgs.instanceCount,
			meshArgs.firstIndex, meshArgs.vertexOffset, meshArgs.firstInstance
		);
	}
}

// Model Bundle Mesh Shader
void ModelBundleMeshShader::AddMeshDetails(
	std::shared_ptr<ModelMS>& model, std::uint32_t modelBufferIndex
) noexcept {
	std::vector<Meshlet>&& meshlets = std::move(model->GetMeshDetailsMS().meshlets);

	const size_t meshletCount = std::size(meshlets);

	m_meshDetails.emplace_back(MeshDetails{
			.modelBufferIndex  = modelBufferIndex,
			.meshletOffset     = static_cast<std::uint32_t>(std::size(m_meshlets)),
			.threadGroupCountX = static_cast<std::uint32_t>(meshletCount)
		});

	std::ranges::move(meshlets, std::back_inserter(m_meshlets));
}

void ModelBundleMeshShader::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	using MS = VkDeviceExtension::VkExtMeshShader;
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& meshDetail : m_meshDetails)
	{
		// Copying the modelIndex and the meshletOffset, so sizeof(std::uint32_t) * 2u.
		constexpr auto pushConstantSize = static_cast<std::uint32_t>(sizeof(std::uint32_t) * 2u);

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_MESH_BIT_EXT, 0u,
			pushConstantSize, &meshDetail.modelBufferIndex
		);

		// Unlike the Compute Shader where we process the data of a model with a thread, here
		// each group handles a Meshlet and its threads handle the vertices and primitives.
		// So, we need a thread group for each Meshlet.
		MS::vkCmdDrawMeshTasksEXT(cmdBuffer, meshDetail.threadGroupCountX, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
}

void ModelBundleMeshShader::CreateBuffers(StagingBufferManager& stagingBufferMan)
{
	const auto meshletBufferSize = static_cast<VkDeviceSize>(std::size(m_meshlets) * sizeof(Meshlet));

	m_meshletBuffer.Create(
		meshletBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
	);

	stagingBufferMan.AddBuffer(std::data(m_meshlets), meshletBufferSize, &m_meshletBuffer, 0u);
}

void ModelBundleMeshShader::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t meshBufferBindingSlot
) const noexcept {
	descriptorBuffer.AddStorageBufferDescriptor(m_meshletBuffer, meshBufferBindingSlot);
}

void ModelBundleMeshShader::CleanupTempData() noexcept
{
	m_meshlets = std::vector<Meshlet>{};
}

// Model Bundle Vertex Shader Indirect
ModelBundleVertexShaderIndirect::ModelBundleVertexShaderIndirect(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices
) : ModelBundle{}, m_modelCount{ 0u }, m_queueIndices{ queueIndices },
	m_argumentBufferSize{ 0u },
	m_counterBufferSize{ static_cast<VkDeviceSize>(sizeof(std::uint32_t)) },
	m_argumentBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_counterBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_counterResetBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_counterResetData{ std::make_unique<std::uint32_t>(0u) }
{}

void ModelBundleVertexShaderIndirect::CreateBuffers(
	std::uint32_t meshCount, std::uint32_t frameCount, StagingBufferManager& stagingBufferMan
) {
	m_modelCount = meshCount;

	constexpr size_t strideSize = sizeof(ModelBundleComputeShaderIndirect::Argument);
	m_argumentBufferSize        = static_cast<VkDeviceSize>(m_modelCount * strideSize);

	const VkDeviceSize argumentBufferTotalSize = m_argumentBufferSize * frameCount;
	const VkDeviceSize counterBufferTotalSize  = m_counterBufferSize * frameCount;

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

[[nodiscard]]

void ModelBundleComputeShaderIndirect::AddMeshDetails(
	const std::shared_ptr<ModelVS>& model, std::uint32_t modelBufferIndex
) noexcept {
	m_indirectArguments.emplace_back(
		Argument{
			.modelBufferIndex  = modelBufferIndex,
			.indirectArguments = ModelBundle::GetDrawIndexedIndirectCommand(model)
		}
	);
}

void ModelBundleComputeShaderIndirect::CreateBuffers(StagingBufferManager& stagingBufferMan)
{
	constexpr size_t strideSize = sizeof(ModelBundleComputeShaderIndirect::Argument);
	const auto argumentCount    = static_cast<std::uint32_t>(std::size(m_indirectArguments));
	m_cullingData->commandCount = argumentCount;

	// ThreadBlockSize is the number of threads in a thread group. If the argumentCount/ModelCount
	// is more than the BlockSize then dispatch more groups. Ex: Threads 64, Model 60 = Group 1
	// Threads 64, Model 65 = Group 2.
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

// Model Buffers
void ModelBuffers::CreateBuffer(size_t modelCount)
{
	constexpr size_t strideSize = GetStride();

	m_modelBuffersInstanceSize              = static_cast<VkDeviceSize>(strideSize * modelCount);
	const VkDeviceSize modelBufferTotalSize = m_modelBuffersInstanceSize * m_bufferInstanceCount;

	m_modelBuffers.Create(modelBufferTotalSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
}

void ModelBuffers::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
) const noexcept {
	const auto bufferOffset = static_cast<VkDeviceAddress>(frameIndex * m_modelBuffersInstanceSize);

	descriptorBuffer.AddStorageBufferDescriptor(
		m_modelBuffers, bindingSlot, bufferOffset, m_modelBuffersInstanceSize
	);
}

size_t ModelBuffers::AddModel(std::shared_ptr<Model>&& model)
{
	auto oModelIndex = m_models.GetFirstAvailableIndex();

	if (oModelIndex)
	{
		const size_t modelIndex = oModelIndex.value();

		m_models.UpdateElement(modelIndex, std::move(model));

		return modelIndex;
	}
	else
	{
		const size_t modelIndex    = GetCount();

		m_models.AddNewElement(std::move(model));

		const size_t newModelCount = GetCount() + GetExtraModelAllocationCount();

		m_models.ReserveNewElements(newModelCount);

		CreateBuffer(newModelCount);

		return modelIndex;
	}
}

std::vector<size_t> ModelBuffers::AddModels(std::vector<std::shared_ptr<Model>>&& models)
{
	std::vector<size_t> freeIndices = m_models.GetAvailableIndices();

	for (size_t index = 0u; index < std::size(freeIndices); ++index)
	{
		const size_t freeIndex = freeIndices.at(index);

		m_models.UpdateElement(freeIndex, std::move(models.at(index)));
	}

	const size_t newModelCount  = std::size(models);
	const size_t freeIndexCount = std::size(freeIndices);

	if (newModelCount > freeIndexCount)
	{
		for (size_t index = freeIndexCount; index < newModelCount; ++index)
		{
			m_models.AddNewElement(std::move(models.at(index)));

			freeIndices.emplace_back(index);
		}

		const size_t newExtraModelCount = newModelCount + GetExtraModelAllocationCount();

		m_models.ReserveNewElements(newExtraModelCount);

		CreateBuffer(newExtraModelCount);
	}

	// Now these are the new used indices.
	freeIndices.resize(newModelCount);

	return freeIndices;
}

void ModelBuffers::RemoveModel(size_t index) noexcept
{
	m_models.RemoveModel(index, &std::shared_ptr<Model>::reset);
}

void ModelBuffers::Update(VkDeviceSize bufferIndex) const
{
	std::uint8_t* bufferOffset  = m_modelBuffers.CPUHandle() + bufferIndex * m_modelBuffersInstanceSize;
	constexpr size_t strideSize = GetStride();
	size_t modelOffset          = 0u;

	auto& models = m_models.Get();

	for (auto& model : models)
	{
		const ModelData modelData{
			.modelMatrix   = model->GetModelMatrix(),
			.modelOffset   = model->GetModelOffset(),
			.materialIndex = model->GetMaterialIndex(),
		};

		memcpy(bufferOffset + modelOffset, &modelData, strideSize);
		modelOffset += strideSize;
	}
}