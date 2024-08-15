#include <ModelManager.hpp>
#include <VectorToSharedPtr.hpp>

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
void ModelBundleVSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundleVS> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBufferIndices = std::move(modelBufferIndices);
	m_modelBundle        = std::move(bundle);
}

void ModelBundleVSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();
	const auto& models        = m_modelBundle->GetModels();

	for(size_t index = 0; index < std::size(models); ++index)
	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
			pushConstantSize, &m_modelBufferIndices[index]
		);

		const VkDrawIndexedIndirectCommand meshArgs = GetDrawIndexedIndirectCommand(models[index]);

		vkCmdDrawIndexed(
			cmdBuffer, meshArgs.indexCount, meshArgs.instanceCount,
			meshArgs.firstIndex, meshArgs.vertexOffset, meshArgs.firstInstance
		);
	}
}

// Model Bundle MS
void ModelBundleMSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundleMS> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBundle        = std::move(bundle);
	m_modelBufferIndices = std::move(modelBufferIndices);
}

void ModelBundleMSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, const PipelineLayout& pipelineLayout
) const noexcept {
	/*
	using MS = VkDeviceExtension::VkExtMeshShader;

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();
	const auto& models        = m_modelBundle->GetModels();

	for (size_t index = 0u; index < std::size(models); ++index)
	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		const RenderData msConstants
		{
			//.modelBufferIndex = modelDetail.modelBufferIndex,
			//.meshletOffset    = modelDetail.meshletOffset
		};

		constexpr std::uint32_t offset = MeshManagerMeshShader::GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout.Get(), VK_SHADER_STAGE_MESH_BIT_EXT, offset,
			pushConstantSize, &msConstants
		);

		// Unlike the Compute Shader where we process the data of a model with a thread, here
		// each group handles a Meshlet and its threads handle the vertices and primitives.
		// So, we need a thread group for each Meshlet.
		//MS::vkCmdDrawMeshTasksEXT(cmdBuffer, modelDetail.threadGroupCountX, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
	*/
}

void ModelBundleMSIndividual::CreateBuffers(
	StagingBufferManager& stagingBufferMan, SharedBuffer& meshletSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer
) {
	/*
	const auto meshletBufferSize = static_cast<VkDeviceSize>(std::size(m_meshlets) * sizeof(Meshlet));

	m_meshletSharedData = meshletSharedBuffer.AllocateAndGetSharedData(meshletBufferSize, tempBuffer);

	std::shared_ptr<std::uint8_t[]> tempDataBuffer = CopyVectorToSharedPtr(m_meshlets);
	m_meshlets = std::vector<Meshlet>{};

	stagingBufferMan.AddBuffer(
		std::move(tempDataBuffer), meshletBufferSize, m_meshletSharedData.bufferData,
		m_meshletSharedData.offset,
		QueueType::GraphicsQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT,
		tempBuffer
	);
	*/
}

// Model Bundle VS Indirect
ModelBundleVSIndirect::ModelBundleVSIndirect()
	: ModelBundle{},
	m_argumentOutputSharedData{ nullptr, 0u, 0u }, m_counterSharedData{ nullptr, 0u, 0u },
	m_modelIndicesSharedData{ nullptr, 0u, 0u }, m_modelIndices{}, m_modelBundle{}
{}

void ModelBundleVSIndirect::SetModelBundle(
	std::shared_ptr<ModelBundleVS> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBundle  = std::move(bundle);
	m_modelIndices = std::move(modelBufferIndices);
}

void ModelBundleVSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan,
	std::vector<SharedBuffer>& argumentOutputSharedBuffers,
	std::vector<SharedBuffer>& counterSharedBuffers, SharedBuffer& modelIndicesBuffer,
	TemporaryDataBufferGPU& tempBuffer
) {
	constexpr size_t argStrideSize      = sizeof(VkDrawIndexedIndirectCommand);
	constexpr size_t indexStrideSize    = sizeof(std::uint32_t);
	const std::uint32_t modelCount      = GetModelCount();
	const auto argumentOutputBufferSize = static_cast<VkDeviceSize>(modelCount * argStrideSize);
	const auto modelIndiceBufferSize    = static_cast<VkDeviceSize>(modelCount * indexStrideSize);

	for (auto& argumentOutputSharedBuffer : argumentOutputSharedBuffers)
		m_argumentOutputSharedData = argumentOutputSharedBuffer.AllocateAndGetSharedData(
			argumentOutputBufferSize, tempBuffer
		);

	for (auto& counterSharedBuffer : counterSharedBuffers)
		m_counterSharedData = counterSharedBuffer.AllocateAndGetSharedData(s_counterBufferSize, tempBuffer);

	m_modelIndicesSharedData = modelIndicesBuffer.AllocateAndGetSharedData(modelIndiceBufferSize, tempBuffer);

	std::shared_ptr<std::uint8_t[]> tempDataBuffer = CopyVectorToSharedPtr(m_modelIndices);

	stagingBufferMan.AddBuffer(
		std::move(tempDataBuffer), modelIndiceBufferSize, m_modelIndicesSharedData.bufferData, 0u,
		tempBuffer
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
		GetModelCount(), strideSize
	);
}

// Model Bundle CS Indirect
ModelBundleCSIndirect::ModelBundleCSIndirect()
	: m_argumentInputSharedData{ nullptr, 0u, 0u }, m_cullingSharedData{ nullptr, 0u, 0u },
	m_modelBundleIndexSharedData{ nullptr, 0u, 0u },
	m_cullingData{
		std::make_unique<CullingData>(
			CullingData{
				.commandCount  = 0u,
				.commandOffset = 0u
			}
		)
	}, m_modelBundle{}, m_bundleID{ std::numeric_limits<std::uint32_t>::max() }
{}

void ModelBundleCSIndirect::SetModelBundle(std::shared_ptr<ModelBundleVS> bundle) noexcept
{
	m_modelBundle = std::move(bundle);
}

void ModelBundleCSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan, SharedBuffer& argumentSharedBuffer,
	SharedBuffer& cullingSharedBuffer, SharedBuffer& modelBundleIndexSharedBuffer,
	TemporaryDataBufferGPU& tempBuffer
) {
	std::vector<VkDrawIndexedIndirectCommand> indirectArguments{};

	{
		const auto& models      = m_modelBundle->GetModels();
		const size_t modelCount = std::size(models);

		indirectArguments.resize(modelCount);

		for (size_t index = 0u; index < modelCount; ++index)
			indirectArguments[index] = ModelBundle::GetDrawIndexedIndirectCommand(models[index]);
	}

	constexpr size_t strideSize  = sizeof(VkDrawIndexedIndirectCommand);
	const auto argumentCount     = static_cast<std::uint32_t>(std::size(indirectArguments));
	m_cullingData->commandCount  = argumentCount;

	const auto argumentBufferSize = static_cast<VkDeviceSize>(strideSize * argumentCount);
	const auto cullingDataSize    = static_cast<VkDeviceSize>(sizeof(CullingData));
	const auto modelIndexDataSize = static_cast<VkDeviceSize>(sizeof(std::uint32_t) * argumentCount);

	m_argumentInputSharedData    = argumentSharedBuffer.AllocateAndGetSharedData(
		argumentBufferSize, tempBuffer
	);
	m_cullingSharedData          = cullingSharedBuffer.AllocateAndGetSharedData(cullingDataSize, tempBuffer);
	m_modelBundleIndexSharedData = modelBundleIndexSharedBuffer.AllocateAndGetSharedData(
		modelIndexDataSize, tempBuffer
	);

	const auto modelBundleIndex = GetModelBundleIndex();

	m_cullingData->commandOffset
		= static_cast<std::uint32_t>(m_argumentInputSharedData.offset / strideSize);

	std::shared_ptr<std::uint8_t[]> indirectArgumentData = CopyVectorToSharedPtr(indirectArguments);

	// Each thread will process a single model independently. And since we are trying to
	// cull all of the models across all of the bundles with a single call to dispatch, we can't
	// set the index as constantData per bundle. So, we will be giving each model the index
	// of its bundle so each thread can work independently.
	auto modelIndices = std::vector<std::uint32_t>(argumentCount, modelBundleIndex);
	std::shared_ptr<std::uint8_t[]> modelIndicesData = CopyVectorToSharedPtr(modelIndices);

	stagingBufferMan.AddBuffer(
		std::move(indirectArgumentData), argumentBufferSize, m_argumentInputSharedData.bufferData,
		m_argumentInputSharedData.offset,
		QueueType::ComputeQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		tempBuffer
	);
	stagingBufferMan.AddBuffer(
		std::move(m_cullingData), cullingDataSize, m_cullingSharedData.bufferData,
		m_cullingSharedData.offset,
		QueueType::ComputeQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		tempBuffer
	);
	stagingBufferMan.AddBuffer(
		std::move(modelIndicesData), modelIndexDataSize,
		m_modelBundleIndexSharedData.bufferData, m_modelBundleIndexSharedData.offset,
		QueueType::ComputeQueue, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
		tempBuffer
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
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot,
	size_t setLayoutIndex
) const {
	const auto bufferOffset = static_cast<VkDeviceAddress>(frameIndex * m_modelBuffersInstanceSize);

	descriptorBuffer.SetStorageBufferDescriptor(
		m_buffers, bindingSlot, setLayoutIndex, 0u, bufferOffset, m_modelBuffersInstanceSize
	);
}

void ModelBuffers::SetFragmentDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot,
	size_t setLayoutIndex
) const {
	const auto bufferOffset
		= static_cast<VkDeviceAddress>(frameIndex * m_modelBuffersFragmentInstanceSize);

	descriptorBuffer.SetStorageBufferDescriptor(
		m_fragmentModelBuffers, bindingSlot, setLayoutIndex, 0u, bufferOffset, m_modelBuffersFragmentInstanceSize
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
	m_graphicsPipelineLayout.Create(descriptorBuffer.GetLayouts());
}

void ModelManagerVSIndividual::ConfigureModelBundle(
	ModelBundleVSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundleVS>&& modelBundle, TemporaryDataBufferGPU&// Not needed in this system.
) const noexcept {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerVSIndividual::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles[bundleIndex];

	const auto& modelIndices = modelBundle.GetIndices();

	for (const auto& modelIndex : modelIndices)
		m_modelBuffers.Remove(modelIndex);
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
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	// The vertex and index buffer should be guarded with the tempData mutex, but
	// I am not doing it here, because the ConfigureMeshBundle function call is already
	// being guarded by the tempData mutex.
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer,
		tempBuffer
	);
}

void ModelManagerVSIndividual::CopyTempData(const VKCommandBuffer& transferCmdBuffer) noexcept
{
	if (m_tempCopyNecessary)
	{
		m_indexBuffer.CopyOldBuffer(transferCmdBuffer);
		m_vertexBuffer.CopyOldBuffer(transferCmdBuffer);

		m_tempCopyNecessary = false;
	}
}

void ModelManagerVSIndividual::SetDescriptorBufferLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers,
	size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
) {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];

		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, vsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
	}
}

void ModelManagerVSIndividual::SetDescriptorBuffer(
	std::vector<VkDescriptorBuffer>& descriptorBuffers,
	size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
) {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot, vsSetLayoutIndex
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex
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
	m_meshIndexBuffer{ device, memoryManager, frameCount }, m_meshDetailsBuffer{ device, memoryManager },
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
	m_dispatchXCount{ 0u }, m_argumentCount{ 0u }, m_modelBundlesCS{}
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
	m_graphicsPipelineLayout.Create(descriptorBuffer.GetLayouts());
}

void ModelManagerVSIndirect::CreatePipelineCS(const VkDescriptorBuffer& descriptorBuffer)
{
	// Push constants needs to be serialised according to the shader stages
	constexpr auto pushConstantSize = GetConstantBufferSize();

	m_pipelineLayoutCS.AddPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, pushConstantSize);

	m_pipelineLayoutCS.Create(descriptorBuffer.GetLayouts());
}

void ModelManagerVSIndirect::ShaderPathSet()
{
	// Must create the pipeline object after the shader path has been set.
	m_computePipeline.Create(
		m_device, m_pipelineLayoutCS, L"VertexShaderCSIndirect", m_shaderPath
	);
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
	std::shared_ptr<ModelBundleVS>&& modelBundle, TemporaryDataBufferGPU& tempBuffer
) {
	ModelBundleCSIndirect modelBundleCS{};

	modelBundleCS.SetModelBundle(modelBundle);
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));

	modelBundleCS.SetID(static_cast<std::uint32_t>(modelBundleObj.GetID()));

	modelBundleObj.CreateBuffers(
		*m_stagingBufferMan, m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesBuffer,
		tempBuffer
	);

	UpdateCounterResetValues();

	modelBundleCS.CreateBuffers(
		*m_stagingBufferMan, m_argumentInputBuffer, m_cullingDataBuffer, m_modelBundleIndexBuffer,
		tempBuffer
	);

	const auto modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

	m_meshIndexBuffer.Add(modelBundleIndexInBuffer);

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	m_argumentCount += modelBundleObj.GetModelCount();

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
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, m_meshBoundsBuffer,
		tempBuffer, QueueType::ComputeQueue, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
	);

	// This function is also used by the the Add function. Calling it early
	// here should make it so it won't be called again the in Add function.
	// But the returned value should be the same.
	const size_t meshIndex = m_meshBundles.GetNextFreeIndex();

	BoundsDetails details = meshManager.GetBoundsDetails();
	m_meshDetailsBuffer.Add(meshIndex, details);
}

void ModelManagerVSIndirect::_updatePerFrame(VkDeviceSize frameIndex) const noexcept
{
		std::uint8_t* bufferOffsetPtr = m_meshIndexBuffer.GetInstancePtr(frameIndex);
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
			const std::uint32_t meshIndex        = bundle.GetMeshIndex();

			const std::uint32_t modelBundleIndex = bundle.GetModelBundleIndex();

			bufferOffset = strideSize * modelBundleIndex;

			memcpy(bufferOffsetPtr + bufferOffset, &meshIndex, strideSize);
		}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);

		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, vsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelIndicesVSBindingSlot, vsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
) const {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot, vsSetLayoutIndex
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesBuffer.GetBuffer(), s_modelIndicesVSBindingSlot, vsSetLayoutIndex, 0u
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);

		descriptorBuffer.AddBinding(
			s_modelBuffersComputeBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
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
			s_modelIndicesCSBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_modelBundleIndexBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshBoundingBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshIndexBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshDetailsBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferCSOfModels(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersComputeBindingSlot, csSetLayoutIndex
		);

		descriptorBuffer.SetStorageBufferDescriptor(
			m_argumentInputBuffer.GetBuffer(), s_argumentInputBufferBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_cullingDataBuffer.GetBuffer(), s_cullingDataBufferBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_argumentOutputBuffers.at(index).GetBuffer(), s_argumenOutputBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_counterBuffers.at(index).GetBuffer(), s_counterBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesBuffer.GetBuffer(), s_modelIndicesCSBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelBundleIndexBuffer.GetBuffer(), s_modelBundleIndexBindingSlot, csSetLayoutIndex, 0u
		);

		m_meshIndexBuffer.SetDescriptorBuffer(descriptorBuffer, s_meshIndexBindingSlot, csSetLayoutIndex);
		m_meshDetailsBuffer.SetDescriptorBuffer(descriptorBuffer, s_meshDetailsBindingSlot, csSetLayoutIndex);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferCSOfMeshes(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const {
	for (auto& descriptorBuffer : descriptorBuffers)
		descriptorBuffer.SetStorageBufferDescriptor(
			m_meshBoundsBuffer.GetBuffer(), s_meshBoundingBindingSlot, csSetLayoutIndex, 0u
		);
}

void ModelManagerVSIndirect::Dispatch(const VKCommandBuffer& computeBuffer) const noexcept
{
	VkCommandBuffer cmdBuffer = computeBuffer.Get();

	m_computePipeline.Bind(computeBuffer);

	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		constexpr ConstantData constantData{
			.maxXBounds = XBOUNDS,
			.maxYBounds = YBOUNDS,
			.maxZBounds = ZBOUNDS
		};

		vkCmdPushConstants(
			cmdBuffer, m_pipelineLayoutCS.Get(), VK_SHADER_STAGE_COMPUTE_BIT, 0u,
			pushConstantSize, &constantData
		);
	}

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
	if (m_tempCopyNecessary)
	{
		m_argumentInputBuffer.CopyOldBuffer(transferBuffer);
		m_cullingDataBuffer.CopyOldBuffer(transferBuffer);
		m_modelIndicesBuffer.CopyOldBuffer(transferBuffer);
		m_vertexBuffer.CopyOldBuffer(transferBuffer);
		m_indexBuffer.CopyOldBuffer(transferBuffer);
		m_modelBundleIndexBuffer.CopyOldBuffer(transferBuffer);
		m_meshBoundsBuffer.CopyOldBuffer(transferBuffer);

		// This should be okay, since when adding new stuffs to these, all of the command buffers
		// should be finished before. And they will be copied in the same transfer buffer. So, it
		// be okay to free it when that single transfer buffer has been finished executing.
		for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
		{
			m_argumentOutputBuffers.at(index).CopyOldBuffer(transferBuffer);
			m_counterBuffers.at(index).CopyOldBuffer(transferBuffer);
		}

		m_tempCopyNecessary = false;
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

			std::uint8_t* bufferStart = m_counterResetBuffer.CPUHandle();

			for (size_t offset = 0u; offset < counterBufferSize; offset += counterSize)
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
	}
{}

void ModelManagerMS::ConfigureModelBundle(
	ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundleMS>&& modelBundle, TemporaryDataBufferGPU& tempBuffer
) {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
	modelBundleObj.CreateBuffers(*m_stagingBufferMan, m_meshletBuffer, tempBuffer);
}

void ModelManagerMS::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles.at(bundleIndex);

	const auto& modelIndices = modelBundle.GetIndices();

	for (std::uint32_t modelIndex : modelIndices)
		m_modelBuffers.Remove(modelIndex);

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
	MeshManagerMeshShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_vertexIndicesBuffer,
		m_primIndicesBuffer, tempBuffer
	);
}

void ModelManagerMS::CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer)
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t meshConstantSize  = MeshManagerMeshShader::GetConstantBufferSize();
	constexpr std::uint32_t modelConstantSize = ModelBundleMSIndividual::GetConstantBufferSize();

	m_graphicsPipelineLayout.AddPushConstantRange(
		VK_SHADER_STAGE_MESH_BIT_EXT, meshConstantSize + modelConstantSize
	);
	m_graphicsPipelineLayout.Create(descriptorBuffer.GetLayouts());
}

void ModelManagerMS::CopyTempBuffers(const VKCommandBuffer& transferBuffer) noexcept
{
	if (m_tempCopyNecessary)
	{
		m_meshletBuffer.CopyOldBuffer(transferBuffer);
		m_vertexBuffer.CopyOldBuffer(transferBuffer);
		m_vertexIndicesBuffer.CopyOldBuffer(transferBuffer);
		m_primIndicesBuffer.CopyOldBuffer(transferBuffer);

		m_tempCopyNecessary = false;
	}
}

void ModelManagerMS::SetDescriptorBufferLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex, size_t fsSetLayoutIndex
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);

		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, msSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descriptorBuffer.AddBinding(
			s_meshletBufferBindingSlot, msSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_vertexBufferBindingSlot, msSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_vertexIndicesBufferBindingSlot, msSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_primIndicesBufferBindingSlot, msSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_MESH_BIT_EXT
		);
	}
}

void ModelManagerMS::SetDescriptorBufferOfModels(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex, size_t fsSetLayoutIndex
) const {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot, msSetLayoutIndex
		);
		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_meshletBuffer.GetBuffer(), s_meshletBufferBindingSlot, msSetLayoutIndex, 0u
		);
	}
}

void ModelManagerMS::SetDescriptorBufferOfMeshes(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex
) const {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
	{
		descriptorBuffer.SetStorageBufferDescriptor(
			m_vertexBuffer.GetBuffer(), s_vertexBufferBindingSlot, msSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_vertexIndicesBuffer.GetBuffer(), s_vertexIndicesBufferBindingSlot, msSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_primIndicesBuffer.GetBuffer(), s_primIndicesBufferBindingSlot, msSetLayoutIndex, 0u
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
