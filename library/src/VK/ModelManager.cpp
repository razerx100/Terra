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

// Model Bundle VS Individual
void ModelBundleVSIndividual::AddModelDetails(
	const std::shared_ptr<ModelVS>& model, std::uint32_t modelBufferIndex
) noexcept {
	m_modelDetails.emplace_back(
		ModelDetails{
			.modelBufferIndex = modelBufferIndex,
			.indexedArguments = GetDrawIndexedIndirectCommand(model)
		}
	);
}

void ModelBundleVSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelDetail : m_modelDetails)
	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
			pushConstantSize, &modelDetail.modelBufferIndex
		);

		const VkDrawIndexedIndirectCommand& meshArgs = modelDetail.indexedArguments;
		vkCmdDrawIndexed(
			cmdBuffer, meshArgs.indexCount, meshArgs.instanceCount,
			meshArgs.firstIndex, meshArgs.vertexOffset, meshArgs.firstInstance
		);
	}
}

// Model Bundle MS
void ModelBundleMS::AddModelDetails(
	std::shared_ptr<ModelMS>& model, std::uint32_t modelBufferIndex
) noexcept {
	std::vector<Meshlet>&& meshlets = std::move(model->GetMeshDetailsMS().meshlets);

	const size_t meshletCount = std::size(meshlets);

	m_modelDetails.emplace_back(ModelDetails{
			.modelBufferIndex  = modelBufferIndex,
			.meshletOffset     = static_cast<std::uint32_t>(std::size(m_meshlets)),
			.threadGroupCountX = static_cast<std::uint32_t>(meshletCount)
		});

	std::ranges::move(meshlets, std::back_inserter(m_meshlets));
}

void ModelBundleMS::Draw(
	const VKCommandBuffer& graphicsBuffer, const PipelineLayout& pipelineLayout
) const noexcept {
	using MS = VkDeviceExtension::VkExtMeshShader;
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelDetail : m_modelDetails)
	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		const RenderData msConstants
		{
			.modelBufferIndex = modelDetail.modelBufferIndex,
			.meshletOffset    = modelDetail.meshletOffset
		};

		constexpr std::uint32_t offset = MeshManagerMeshShader::GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout.Get(), VK_SHADER_STAGE_MESH_BIT_EXT, offset,
			pushConstantSize, &msConstants
		);

		// Unlike the Compute Shader where we process the data of a model with a thread, here
		// each group handles a Meshlet and its threads handle the vertices and primitives.
		// So, we need a thread group for each Meshlet.
		MS::vkCmdDrawMeshTasksEXT(cmdBuffer, modelDetail.threadGroupCountX, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
}

void ModelBundleMS::CreateBuffers(
	StagingBufferManager& stagingBufferMan, SharedBuffer& meshletSharedBuffer,
	std::deque<TempData>& tempDataContainer
) {
	const auto meshletBufferSize = static_cast<VkDeviceSize>(std::size(m_meshlets) * sizeof(Meshlet));

	m_meshletSharedData = meshletSharedBuffer.AllocateAndGetSharedData(meshletBufferSize);

	TempData& tempData = tempDataContainer.emplace_back(TempData{ std::move(m_meshlets) });

	stagingBufferMan.AddBuffer(
		std::data(tempData.meshlets), meshletBufferSize, m_meshletSharedData.bufferData,
		m_meshletSharedData.offset,
		QueueType::GraphicsQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT
	);
}

// Model Bundle VS Indirect
ModelBundleVSIndirect::ModelBundleVSIndirect()
	: ModelBundle{}, m_modelCount{ 0u },
	m_argumentOutputSharedData{ nullptr, 0u, 0u }, m_counterSharedData{ nullptr, 0u, 0u },
	m_modelIndicesSharedData{ nullptr, 0u, 0u }, m_modelIndices{}
{}

void ModelBundleVSIndirect::AddModelDetails(std::uint32_t modelBufferIndex) noexcept
{
	m_modelIndices.emplace_back(modelBufferIndex);
	++m_modelCount;
}

void ModelBundleVSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan,
	std::vector<SharedBuffer>& argumentOutputSharedBuffers,
	std::vector<SharedBuffer>& counterSharedBuffers, SharedBuffer& modelIndicesBuffer
) {
	constexpr size_t argStrideSize      = sizeof(VkDrawIndexedIndirectCommand);
	constexpr size_t indexStrideSize    = sizeof(std::uint32_t);
	const auto argumentOutputBufferSize = static_cast<VkDeviceSize>(m_modelCount * argStrideSize);
	const auto modelIndiceBufferSize    = static_cast<VkDeviceSize>(m_modelCount * indexStrideSize);

	// The shared data for every instance should be the same. So, we can use the last one for the
	// other ones as well. And as we are not using the buffer object for the stagingBufferMan,
	// it should be fine.
	for (auto& argumentOutputSharedBuffer : argumentOutputSharedBuffers)
		m_argumentOutputSharedData = argumentOutputSharedBuffer.AllocateAndGetSharedData(
			argumentOutputBufferSize
		);

	for (auto& counterSharedBuffer : counterSharedBuffers)
		m_counterSharedData = counterSharedBuffer.AllocateAndGetSharedData(s_counterBufferSize);

	m_modelIndicesSharedData = modelIndicesBuffer.AllocateAndGetSharedData(modelIndiceBufferSize);

	stagingBufferMan.AddBuffer(
		std::data(m_modelIndices), modelIndiceBufferSize, m_modelIndicesSharedData.bufferData, 0u
	);
}

void ModelBundleVSIndirect::Draw(const VKCommandBuffer& graphicsBuffer) const noexcept
{
	constexpr auto strideSize = static_cast<std::uint32_t>(sizeof(VkDrawIndexedIndirectCommand));

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	vkCmdDrawIndexedIndirectCount(
		cmdBuffer,
		m_argumentOutputSharedData.bufferData->Get(), m_argumentOutputSharedData.offset,
		m_counterSharedData.bufferData->Get(), m_counterSharedData.offset,
		m_modelCount, strideSize
	);
}

// Model Bundle CS Indirect
ModelBundleCSIndirect::ModelBundleCSIndirect()
	: m_argumentInputSharedData{ nullptr, 0u, 0u }, m_cullingSharedData{ nullptr, 0u, 0u },
	m_modelBundleIndexSharedData{ nullptr, 0u, 0u }, m_indirectArguments{},
	m_cullingData{
		std::make_unique<CullingData>(
			CullingData{
				.commandCount  = 0u,
				.commandOffset = 0u,
				.xBounds       = XBOUNDS,
				.yBounds       = YBOUNDS,
				.zBounds       = ZBOUNDS
			}
		)
	}, m_bundleID{ std::numeric_limits<std::uint32_t>::max() }
{}

void ModelBundleCSIndirect::AddModelDetails(const std::shared_ptr<ModelVS>& model) noexcept
{
	m_indirectArguments.emplace_back(ModelBundle::GetDrawIndexedIndirectCommand(model));
}

void ModelBundleCSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan, SharedBuffer& argumentSharedBuffer,
	SharedBuffer& cullingSharedBuffer, SharedBuffer& modelBundleIndexSharedBuffer,
	std::deque<TempData>& tempDataContainer
) {
	constexpr size_t strideSize  = sizeof(VkDrawIndexedIndirectCommand);
	const auto argumentCount     = static_cast<std::uint32_t>(std::size(m_indirectArguments));
	m_cullingData->commandCount  = argumentCount;

	const auto argumentBufferSize = static_cast<VkDeviceSize>(strideSize * argumentCount);
	const auto cullingDataSize    = static_cast<VkDeviceSize>(sizeof(CullingData));
	const auto modelIndexDataSize = static_cast<VkDeviceSize>(sizeof(std::uint32_t) * argumentCount);

	m_argumentInputSharedData    = argumentSharedBuffer.AllocateAndGetSharedData(argumentBufferSize);
	m_cullingSharedData          = cullingSharedBuffer.AllocateAndGetSharedData(cullingDataSize);
	m_modelBundleIndexSharedData = modelBundleIndexSharedBuffer.AllocateAndGetSharedData(
		modelIndexDataSize
	);

	const auto modelBundleIndex = GetModelBundleIndex();

	m_cullingData->commandOffset
		= static_cast<std::uint32_t>(m_argumentInputSharedData.offset / strideSize);

	TempData& tempData = tempDataContainer.emplace_back(
		TempData{
			.indirectArguments  = std::move(m_indirectArguments),
			// Each thread will process a single model independently. And since we are trying to
			// cull all the models across all the bundles with a single call to dispatch, we can't
			// set the index as constantData per bundle. So, we will be giving each model the index
			// of its bundle so each thread can work independently.
			.modelBundleIndices = std::vector<std::uint32_t>(argumentCount, modelBundleIndex),
			.cullingData        = std::move(m_cullingData)
		}
	);

	stagingBufferMan.AddBuffer(
		std::data(tempData.indirectArguments), argumentBufferSize, m_argumentInputSharedData.bufferData,
		m_argumentInputSharedData.offset,
		QueueType::ComputeQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
	);
	stagingBufferMan.AddBuffer(
		tempData.cullingData.get(), cullingDataSize, m_cullingSharedData.bufferData,
		m_cullingSharedData.offset,
		QueueType::ComputeQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
	);
	stagingBufferMan.AddBuffer(
		std::data(tempData.modelBundleIndices), modelIndexDataSize,
		m_modelBundleIndexSharedData.bufferData, m_modelBundleIndexSharedData.offset,
		QueueType::ComputeQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
	);
}

// Model Buffers
void ModelBuffers::CreateBuffer(size_t modelCount)
{
	// Vertex Data
	{
		constexpr size_t strideSize = GetVertexStride();

		m_modelBuffersInstanceSize = static_cast<VkDeviceSize>(strideSize * modelCount);
		const VkDeviceSize modelBufferTotalSize = m_modelBuffersInstanceSize * m_bufferInstanceCount;

		m_buffers.Create(modelBufferTotalSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
	}

	// Fragment Data
	{
		constexpr size_t strideSize = GetFragmentStride();

		m_modelBuffersFragmentInstanceSize = static_cast<VkDeviceSize>(strideSize * modelCount);
		const VkDeviceSize modelBufferTotalSize
			= m_modelBuffersFragmentInstanceSize * m_bufferInstanceCount;

		m_fragmentModelBuffers.Create(modelBufferTotalSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
	}
}

void ModelBuffers::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
) const {
	const auto bufferOffset = static_cast<VkDeviceAddress>(frameIndex * m_modelBuffersInstanceSize);

	descriptorBuffer.SetStorageBufferDescriptor(
		m_buffers, bindingSlot, 0u, bufferOffset, m_modelBuffersInstanceSize
	);
}

void ModelBuffers::SetFragmentDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
) const {
	const auto bufferOffset
		= static_cast<VkDeviceAddress>(frameIndex * m_modelBuffersFragmentInstanceSize);

	descriptorBuffer.SetStorageBufferDescriptor(
		m_fragmentModelBuffers, bindingSlot, 0u, bufferOffset, m_modelBuffersFragmentInstanceSize
	);
}

void ModelBuffers::Update(VkDeviceSize bufferIndex) const noexcept
{
	// Vertex Data
	std::uint8_t* vertexBufferOffset  = m_buffers.CPUHandle() + bufferIndex * m_modelBuffersInstanceSize;
	constexpr size_t vertexStrideSize = GetVertexStride();
	size_t vertexModelOffset          = 0u;

	// Fragment Data
	std::uint8_t* fragmentBufferOffset
		= m_fragmentModelBuffers.CPUHandle() + bufferIndex * m_modelBuffersFragmentInstanceSize;
	constexpr size_t fragmentStrideSize = GetFragmentStride();
	size_t fragmentModelOffset          = 0u;

	auto& models = m_elements.Get();

	for (auto& model : models)
	{
		// Vertex Data
		{
			const ModelVertexData modelVertexData{
				.modelMatrix = model->GetModelMatrix(),
				.modelOffset = model->GetModelOffset(),
				.materialIndex = model->GetMaterialIndex()
			};

			memcpy(vertexBufferOffset + vertexModelOffset, &modelVertexData, vertexStrideSize);
			vertexModelOffset += vertexStrideSize;
		}

		// Fragment Data
		{
			const ModelFragmentData modelFragmentData{
				.diffuseTexUVInfo  = model->GetDiffuseUVInfo(),
				.specularTexUVInfo = model->GetSpecularUVInfo(),
				.diffuseTexIndex   = model->GetDiffuseIndex(),
				.specularTexIndex  = model->GetSpecularIndex()
			};

			memcpy(fragmentBufferOffset + fragmentModelOffset, &modelFragmentData, fragmentStrideSize);
			fragmentModelOffset += fragmentStrideSize;
		}
	}
}

// Model Manager VS Individual
ModelManagerVSIndividual::ModelManagerVSIndividual(
	VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount },
	m_vertexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, {}
	}, m_indexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, {}
	}
{}

void ModelManagerVSIndividual::CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer)
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = ModelBundleVSIndividual::GetConstantBufferSize();

	m_graphicsPipelineLayout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
	m_graphicsPipelineLayout.Create(descriptorBuffer.GetLayout());
}

void ModelManagerVSIndividual::ConfigureModel(
	ModelBundleVSIndividual& modelBundleObj, size_t modelIndex, const std::shared_ptr<ModelVS>& model
) const noexcept {
	modelBundleObj.AddModelDetails(model, static_cast<std::uint32_t>(modelIndex));
}

void ModelManagerVSIndividual::ConfigureModelBundle(
	ModelBundleVSIndividual& modelBundleObj,
	const std::vector<size_t>& modelIndices,
	const std::vector<std::shared_ptr<ModelVS>>& modelBundle
) const noexcept {
	const size_t modelCount = std::size(modelBundle);

	for (size_t index = 0u; index < modelCount; ++index)
	{
		const std::shared_ptr<ModelVS>& model = modelBundle.at(index);
		const size_t modelIndex               = modelIndices.at(index);

		modelBundleObj.AddModelDetails(model, static_cast<std::uint32_t>(modelIndex));
	}
}

void ModelManagerVSIndividual::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles.at(bundleIndex);

	const auto& modelDetails = modelBundle.GetDetails();

	for (const auto& modelDetail : modelDetails)
		m_modelBuffers.Remove(modelDetail.modelBufferIndex);
}

void ModelManagerVSIndividual::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	auto& meshManager = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData = meshManager.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData = meshManager.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);
	}
}

void ModelManagerVSIndividual::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager
) {
	// The vertex and index buffer should be guarded with the tempData mutex, but
	// I am not doing it here, because the ConfigureMeshBundle function call is already
	// being guarded by the tempData mutex.
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer,
		m_meshBundleTempData
	);
}

void ModelManagerVSIndividual::CopyTempData(const VKCommandBuffer& transferCmdBuffer) const noexcept
{
	if (m_indexBuffer.DoesTempDataExist())
	{
		m_indexBuffer.CopyOldBuffer(transferCmdBuffer);
	}
	if (m_vertexBuffer.DoesTempDataExist())
	{
		m_vertexBuffer.CopyOldBuffer(transferCmdBuffer);
	}
}

void ModelManagerVSIndividual::_cleanUpTempData() noexcept
{
	if (m_indexBuffer.DoesTempDataExist())
	{
		m_indexBuffer.CleanupTempData();
	}
	if (m_vertexBuffer.DoesTempDataExist())
	{
		m_vertexBuffer.CleanupTempData();
	}
}

void ModelManagerVSIndividual::SetDescriptorBufferLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);

		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}
}

void ModelManagerVSIndividual::SetDescriptorBuffer(std::vector<VkDescriptorBuffer>& descriptorBuffers)
{
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot
		);
	}
}

void ModelManagerVSIndividual::Draw(const VKCommandBuffer& graphicsBuffer) const noexcept
{
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, previousPSOIndex);

		// Mesh
		BindMesh(modelBundle, graphicsBuffer);

		// Model
		modelBundle.Draw(graphicsBuffer, m_graphicsPipelineLayout);
	}
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	QueueIndices3 queueIndices3, std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount }, m_stagingBufferMan{ stagingBufferMan },
	m_argumentInputBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {}
	}, m_argumentOutputBuffers{},
	m_cullingDataBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {}
	}, m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
	m_meshIndexBuffer{ device, memoryManager },
	m_modelIndicesBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndices3>()
	}, m_vertexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, {}
	}, m_indexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, {}
	}, m_modelBundleIndexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {}
	}, m_pipelineLayoutCS{ device }, m_computePipeline{}, m_queueIndices3{ queueIndices3 },
	m_dispatchXCount{ 0u }, m_argumentCount{ 0u }, m_tempGPUDataCopied{ false },
	m_modelBundlesCS{}, m_modelBundleCSTempData{}
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		m_argumentOutputBuffers.emplace_back(
			SharedBuffer{
				m_device, m_memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
		m_counterBuffers.emplace_back(
			SharedBuffer{
				m_device, m_memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndices3>()
			}
		);
	}
}

void ModelManagerVSIndirect::CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer)
{
	m_graphicsPipelineLayout.Create(descriptorBuffer.GetLayout());
}

void ModelManagerVSIndirect::CreatePipelineCS(const VkDescriptorBuffer& descriptorBuffer)
{
	m_pipelineLayoutCS.Create(descriptorBuffer.GetLayout());

	m_computePipeline.Create(m_device, m_pipelineLayoutCS, m_shaderPath);
}

void ModelManagerVSIndirect::UpdateDispatchX() noexcept
{
	// ThreadBlockSize is the number of threads in a thread group. If the argumentCount/ModelCount
	// is more than the BlockSize then dispatch more groups. Ex: Threads 64, Model 60 = Group 1
	// Threads 64, Model 65 = Group 2.

	m_dispatchXCount = static_cast<std::uint32_t>(std::ceil(m_argumentCount / THREADBLOCKSIZE));
}

void ModelManagerVSIndirect::ConfigureModel(
	ModelBundleVSIndirect& modelBundleObj, size_t modelIndex, const std::shared_ptr<ModelVS>& model
) {
	ModelBundleCSIndirect modelBundleCS{};
	modelBundleCS.AddModelDetails(model);

	{
		std::scoped_lock<std::mutex> tempDataLock{ m_tempDataMutex };

		modelBundleCS.CreateBuffers(
			*m_stagingBufferMan, m_argumentInputBuffer, m_cullingDataBuffer, m_modelBundleIndexBuffer,
			m_modelBundleCSTempData
		);
	}

	const auto index32_t = static_cast<std::uint32_t>(modelIndex);

	modelBundleCS.SetID(index32_t);

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	modelBundleObj.AddModelDetails(index32_t);

	modelBundleObj.CreateBuffers(
		*m_stagingBufferMan, m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesBuffer
	);

	UpdateCounterResetValues();

	++m_argumentCount;

	UpdateDispatchX();
}

void ModelManagerVSIndirect::_setMeshIndex(
	size_t modelBundelVSIndex, std::uint32_t meshBundleID
) {
	// Both should have the same index.
	ModelBundleCSIndirect& modelBundleCS = m_modelBundlesCS.at(modelBundelVSIndex);

	if (m_modelBundles.at(modelBundelVSIndex).GetModelCount())
	{
		const std::uint32_t modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

		m_meshIndexBuffer.Add(modelBundleIndexInBuffer, meshBundleID);
	}
}

void ModelManagerVSIndirect::ConfigureModelBundle(
	ModelBundleVSIndirect& modelBundleObj, const std::vector<size_t>& modelIndices,
	const std::vector<std::shared_ptr<ModelVS>>& modelBundle
) {
	const size_t modelCount = std::size(modelBundle);

	ModelBundleCSIndirect modelBundleCS{};

	for (size_t index = 0u; index < modelCount; ++index)
	{
		const std::shared_ptr<ModelVS>& model = modelBundle.at(index);
		const size_t modelIndex               = modelIndices.at(index);

		modelBundleCS.AddModelDetails(model);

		modelBundleObj.AddModelDetails(static_cast<std::uint32_t>(modelIndex));
	}

	modelBundleCS.SetID(static_cast<std::uint32_t>(modelBundleObj.GetID()));

	modelBundleObj.CreateBuffers(
		*m_stagingBufferMan, m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesBuffer
	);

	UpdateCounterResetValues();

	{
		std::scoped_lock<std::mutex> tempDataLock{ m_tempDataMutex };

		modelBundleCS.CreateBuffers(
			*m_stagingBufferMan, m_argumentInputBuffer, m_cullingDataBuffer, m_modelBundleIndexBuffer,
			m_modelBundleCSTempData
		);
	}

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	m_argumentCount += static_cast<std::uint32_t>(modelCount);

	UpdateDispatchX();
}

void ModelManagerVSIndirect::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles.at(bundleIndex);

	const auto& modelIndices = modelBundle.GetModelIndices();

	for (const auto& modelIndex : modelIndices)
		m_modelBuffers.Remove(modelIndex);

	m_argumentCount -= static_cast<std::uint32_t>(std::size(modelIndices));

	UpdateDispatchX();

	const auto bundleID = static_cast<std::uint32_t>(modelBundle.GetID());

	{
		// All of the shared data instances should have the same offset and size, so it should be
		// fine to relinquish them with the same shared data.
		const SharedBufferData& argumentOutputSharedData = modelBundle.GetArgumentOutputSharedData();

		for (auto& argumentOutputBuffer : m_argumentOutputBuffers)
			argumentOutputBuffer.RelinquishMemory(argumentOutputSharedData);

		const SharedBufferData& counterSharedData = modelBundle.GetCounterSharedData();

		for(auto& counterBuffer : m_counterBuffers)
			counterBuffer.RelinquishMemory(counterSharedData);

		const SharedBufferData& modelIndicesSharedData = modelBundle.GetModelIndicesSharedData();

		m_modelIndicesBuffer.RelinquishMemory(modelIndicesSharedData);
	}

	std::erase_if(
		m_modelBundlesCS,
		[bundleID, &argumentInput = m_argumentInputBuffer,
		&cullingData = m_cullingDataBuffer, &bundleIndices = m_modelBundleIndexBuffer]
		(const ModelBundleCSIndirect& bundle)
		{
			const bool result = bundleID == bundle.GetID();

			if (result)
			{
				argumentInput.RelinquishMemory(bundle.GetArgumentInputSharedData());
				cullingData.RelinquishMemory(bundle.GetCullingSharedData());
				bundleIndices.RelinquishMemory(bundle.GetModelBundleIndexSharedData());
			}

			return result;
		}
	);
}

void ModelManagerVSIndirect::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	auto& meshManager = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData = meshManager.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData = meshManager.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);

		const SharedBufferData& meshBoundsSharedData = meshManager.GetBoundsSharedData();
		m_meshBoundsBuffer.RelinquishMemory(meshBoundsSharedData);
	}
}

void ModelManagerVSIndirect::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleVS> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, m_meshBoundsBuffer,
		m_meshBundleTempData,
		QueueType::ComputeQueue, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
	);
}

void ModelManagerVSIndirect::_cleanUpTempData() noexcept
{
	m_modelBundleCSTempData = std::deque<CSIndirectTempData>{};
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);

		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelIndicesVSBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot
		);
		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesBuffer.GetBuffer(), s_modelIndicesVSBindingSlot, 0u
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);

		descriptorBuffer.AddBinding(
			s_argumentInputBufferBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_cullingDataBufferBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_argumenOutputBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_counterBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelIndicesCSBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBundleIndexBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshBoundingBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshIndicesBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferCSOfModels(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersComputeBindingSlot
		);

		descriptorBuffer.SetStorageBufferDescriptor(
			m_argumentInputBuffer.GetBuffer(), s_argumentInputBufferBindingSlot, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_cullingDataBuffer.GetBuffer(), s_cullingDataBufferBindingSlot, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_argumentOutputBuffers.at(index).GetBuffer(), s_argumenOutputBindingSlot, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_counterBuffers.at(index).GetBuffer(), s_counterBindingSlot, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesBuffer.GetBuffer(), s_modelIndicesCSBindingSlot, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelBundleIndexBuffer.GetBuffer(), s_modelBundleIndexBindingSlot, 0u
		);

		m_meshIndexBuffer.SetDescriptorBuffer(descriptorBuffer, s_meshIndicesBindingSlot);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferCSOfMeshes(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.SetStorageBufferDescriptor(
			m_meshBoundsBuffer.GetBuffer(), s_meshBoundingBindingSlot, 0u
		);
}

void ModelManagerVSIndirect::Dispatch(const VKCommandBuffer& computeBuffer) const noexcept
{
	VkCommandBuffer cmdBuffer = computeBuffer.Get();

	vkCmdDispatch(cmdBuffer, m_dispatchXCount, 1u, 1u);
}

void ModelManagerVSIndirect::Draw(const VKCommandBuffer& graphicsBuffer) const noexcept
{
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, previousPSOIndex);

		// Mesh
		BindMesh(modelBundle, graphicsBuffer);

		// Model
		modelBundle.Draw(graphicsBuffer);
	}
}

void ModelManagerVSIndirect::CopyTempBuffers(const VKCommandBuffer& transferBuffer) noexcept
{
	if (!m_tempGPUDataCopied)
	{
		m_argumentInputBuffer.CopyOldBuffer(transferBuffer);
		m_cullingDataBuffer.CopyOldBuffer(transferBuffer);

		for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
		{
			m_argumentOutputBuffers.at(index).CopyOldBuffer(transferBuffer);
			m_counterBuffers.at(index).CopyOldBuffer(transferBuffer);
		}

		m_tempGPUDataCopied = true;
	}
}

void ModelManagerVSIndirect::ResetCounterBuffer(
	const VKCommandBuffer& transferBuffer, VkDeviceSize frameIndex
) const noexcept {
	transferBuffer.CopyWhole(m_counterResetBuffer, m_counterBuffers.at(frameIndex).GetBuffer());
}

void ModelManagerVSIndirect::UpdateCounterResetValues()
{
	if (!std::empty(m_counterBuffers))
	{
		const SharedBuffer& counterBuffer    = m_counterBuffers.front();

		const VkDeviceSize counterBufferSize = counterBuffer.Size();
		const VkDeviceSize oldCounterSize    = m_counterResetBuffer.BufferSize();

		if (counterBufferSize > oldCounterSize)
		{
			const size_t counterSize = sizeof(std::uint32_t);

			// This should be the source buffer. And should only be accessed from a single type of
			// queue.
			m_counterResetBuffer.Create(counterBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

			constexpr std::uint32_t value = 0u;

			size_t offset             = 0u;
			std::uint8_t* bufferStart = m_counterResetBuffer.CPUHandle();

			for (; offset < counterBufferSize; offset += counterSize)
				memcpy(bufferStart + offset, &value, counterSize);
		}
	}
}

// Model Manager MS.
ModelManagerMS::ModelManagerMS(
	VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount },
	m_stagingBufferMan{ stagingBufferMan },
	m_meshletBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {}
	}, m_vertexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {}
	}, m_vertexIndicesBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {}
	}, m_primIndicesBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {}
	}, m_modelBundleTempData {}
{}

void ModelManagerMS::ConfigureModel(
	ModelBundleMS& modelBundleObj, size_t modelIndex, std::shared_ptr<ModelMS>& model
) {
	modelBundleObj.AddModelDetails(model, static_cast<std::uint32_t>(modelIndex));

	{
		std::scoped_lock<std::mutex> tempDataLock{ m_tempDataMutex };
		modelBundleObj.CreateBuffers(*m_stagingBufferMan, m_meshletBuffer, m_modelBundleTempData);
	}
}

void ModelManagerMS::ConfigureModelBundle(
	ModelBundleMS& modelBundleObj, const std::vector<size_t>& modelIndices,
	std::vector<std::shared_ptr<ModelMS>>& modelBundle
) {
	const size_t modelCount = std::size(modelBundle);

	for (size_t index = 0u; index < modelCount; ++index)
	{
		std::shared_ptr<ModelMS>& model = modelBundle.at(index);
		const size_t modelIndex         = modelIndices.at(index);

		modelBundleObj.AddModelDetails(model, static_cast<std::uint32_t>(modelIndex));
	}

	{
		std::scoped_lock<std::mutex> tempDataLock{ m_tempDataMutex };
		modelBundleObj.CreateBuffers(*m_stagingBufferMan, m_meshletBuffer, m_modelBundleTempData);
	}
}

void ModelManagerMS::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles.at(bundleIndex);

	const auto& modelDetails = modelBundle.GetDetails();

	for (const auto& modelDetail : modelDetails)
		m_modelBuffers.Remove(modelDetail.modelBufferIndex);

	{
		const SharedBufferData& meshletSharedData = modelBundle.GetMeshletSharedData();

		m_meshletBuffer.RelinquishMemory(meshletSharedData);
	}
}

void ModelManagerMS::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	auto& meshManager = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData        = meshManager.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& vertexIndicesSharedData = meshManager.GetVertexIndicesSharedData();
		m_vertexIndicesBuffer.RelinquishMemory(vertexIndicesSharedData);

		const SharedBufferData& primIndicesSharedData   = meshManager.GetPrimIndicesSharedData();
		m_primIndicesBuffer.RelinquishMemory(primIndicesSharedData);
	}
}

void ModelManagerMS::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleMS> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerMeshShader& meshManager
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_vertexIndicesBuffer,
		m_primIndicesBuffer, m_meshBundleTempData
	);
}

void ModelManagerMS::CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer)
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t meshConstantSize  = MeshManagerMeshShader::GetConstantBufferSize();
	constexpr std::uint32_t modelConstantSize = ModelBundleMS::GetConstantBufferSize();

	m_graphicsPipelineLayout.AddPushConstantRange(
		VK_SHADER_STAGE_MESH_BIT_EXT, meshConstantSize + modelConstantSize
	);
	m_graphicsPipelineLayout.Create(descriptorBuffer.GetLayout());
}

void ModelManagerMS::_cleanUpTempData() noexcept
{
	m_modelBundleTempData = std::deque<MSBundleTempData>{};
}

void ModelManagerMS::SetDescriptorBufferLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);

		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshletBufferBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_vertexBufferBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_vertexIndicesBufferBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_primIndicesBufferBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
	}
}

void ModelManagerMS::SetDescriptorBufferOfModels(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot
		);
		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_meshletBuffer.GetBuffer(), s_meshletBufferBindingSlot, 0u
		);
	}
}

void ModelManagerMS::SetDescriptorBufferOfMeshes(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) const {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
	{
		descriptorBuffer.SetStorageBufferDescriptor(
			m_vertexBuffer.GetBuffer(), s_vertexBufferBindingSlot, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_vertexIndicesBuffer.GetBuffer(), s_vertexIndicesBufferBindingSlot, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_primIndicesBuffer.GetBuffer(), s_primIndicesBufferBindingSlot, 0u
		);
	}
}

void ModelManagerMS::Draw(const VKCommandBuffer& graphicsBuffer) const noexcept
{
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, previousPSOIndex);

		{
			const size_t meshIndex                  = modelBundle.GetMeshIndex();
			const MeshManagerMeshShader& meshBundle = m_meshBundles.at(meshIndex);
			constexpr std::uint32_t constBufferSize = MeshManagerMeshShader::GetConstantBufferSize();

			const MeshManagerMeshShader::MeshDetails meshDetails = meshBundle.GetMeshDetails();

			VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

			vkCmdPushConstants(
				cmdBuffer, m_graphicsPipelineLayout.Get(), VK_SHADER_STAGE_MESH_BIT_EXT, 0u,
				constBufferSize, &meshDetails
			);
		}

		// Model
		modelBundle.Draw(graphicsBuffer, m_graphicsPipelineLayout);
	}
}
