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
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	using MS = VkDeviceExtension::VkExtMeshShader;
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelDetail : m_modelDetails)
	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_MESH_BIT_EXT, 0u,
			pushConstantSize, &modelDetail.modelBufferIndex
		);

		// Unlike the Compute Shader where we process the data of a model with a thread, here
		// each group handles a Meshlet and its threads handle the vertices and primitives.
		// So, we need a thread group for each Meshlet.
		MS::vkCmdDrawMeshTasksEXT(cmdBuffer, modelDetail.threadGroupCountX, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
}

void ModelBundleMS::CreateBuffers(StagingBufferManager& stagingBufferMan)
{
	const auto meshletBufferSize = static_cast<VkDeviceSize>(std::size(m_meshlets) * sizeof(Meshlet));

	m_meshletBuffer.Create(
		meshletBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
	);

	stagingBufferMan.AddBuffer(std::data(m_meshlets), meshletBufferSize, &m_meshletBuffer, 0u);
}

void ModelBundleMS::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t meshBufferBindingSlot
) const noexcept {
	descriptorBuffer.SetStorageBufferDescriptor(m_meshletBuffer, meshBufferBindingSlot, 0u);
}

void ModelBundleMS::CleanupTempData() noexcept
{
	m_meshlets = std::vector<Meshlet>{};
}

// Model Bundle VS Indirect
ModelBundleVSIndirect::ModelBundleVSIndirect(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices
) : ModelBundle{}, m_modelCount{ 0u }, m_queueIndices{ queueIndices },
	m_argumentOutputBufferSize{ 0u },
	m_modelIndicesBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT },
	m_argumentOutputSharedData{}, m_counterSharedData{},
	m_modelIndices{}
{}

void ModelBundleVSIndirect::AddModelDetails(std::uint32_t modelBufferIndex) noexcept
{
	m_modelIndices.emplace_back(modelBufferIndex);
	++m_modelCount;
}

void ModelBundleVSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan,
	std::vector<SharedBuffer>& argumentOutputSharedBuffers,
	std::vector<SharedBuffer>& counterSharedBuffers
) {
	constexpr size_t strideSize      = sizeof(VkDrawIndexedIndirectCommand);
	m_argumentOutputBufferSize       = static_cast<VkDeviceSize>(m_modelCount * strideSize);
	const auto modelIndiceBufferSize = static_cast<VkDeviceSize>(m_modelCount * sizeof(std::uint32_t));

	// The shared data for every instance should be the same. So, we can use the last one for the
	// other ones as well. And as we are not using the buffer object for the stagingBufferMan,
	// it should be fine.
	for (auto& argumentOutputSharedBuffer : argumentOutputSharedBuffers)
		m_argumentOutputSharedData = argumentOutputSharedBuffer.AllocateAndGetSharedData(
			m_argumentOutputBufferSize
		);

	for (auto& counterSharedBuffer : counterSharedBuffers)
		m_counterSharedData = counterSharedBuffer.AllocateAndGetSharedData(s_counterBufferSize);
}

void ModelBundleVSIndirect::CreateBuffers(
	StagingBufferManager& stagingBufferMan
) {
	constexpr size_t strideSize      = sizeof(VkDrawIndexedIndirectCommand);
	m_argumentOutputBufferSize       = static_cast<VkDeviceSize>(m_modelCount * strideSize);
	const auto modelIndiceBufferSize = static_cast<VkDeviceSize>(m_modelCount * sizeof(std::uint32_t));

	const std::vector<std::uint32_t> allQueueIndices
		= m_queueIndices.ResolveQueueIndices<QueueIndices3>();

	m_modelIndicesBuffer.Create(
		modelIndiceBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, allQueueIndices
	);

	stagingBufferMan.AddBuffer(
		std::data(m_modelIndices), modelIndiceBufferSize, &m_modelIndicesBuffer, 0u
	);
}

void ModelBundleVSIndirect::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t modelIndicesBindingSlot
) const noexcept {
	descriptorBuffer.SetStorageBufferDescriptor(m_modelIndicesBuffer, modelIndicesBindingSlot, 0u);
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
	m_indirectArguments{},
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
	SharedBuffer& cullingSharedBuffer
) {
	constexpr size_t strideSize  = sizeof(VkDrawIndexedIndirectCommand);
	const auto argumentCount     = static_cast<std::uint32_t>(std::size(m_indirectArguments));
	m_cullingData->commandCount  = argumentCount;

	const auto argumentBufferSize = static_cast<VkDeviceSize>(strideSize * argumentCount);
	const auto cullingDataSize    = static_cast<VkDeviceSize>(sizeof(CullingData));

	m_argumentInputSharedData = argumentSharedBuffer.AllocateAndGetSharedData(argumentBufferSize);
	m_cullingSharedData       = cullingSharedBuffer.AllocateAndGetSharedData(cullingDataSize);

	m_cullingData->commandOffset
		= static_cast<std::uint32_t>(m_argumentInputSharedData.offset / strideSize);

	stagingBufferMan.AddBuffer(
		std::data(m_indirectArguments), argumentBufferSize, m_argumentInputSharedData.bufferData,
		m_argumentInputSharedData.offset
	);
	stagingBufferMan.AddBuffer(
		m_cullingData.get(), cullingDataSize, m_cullingSharedData.bufferData, m_cullingSharedData.offset
	);
}

void ModelBundleCSIndirect::CleanupTempData() noexcept
{
	m_indirectArguments = std::vector<VkDrawIndexedIndirectCommand>{};
	m_cullingData.reset();
}

// Model Buffers
void ModelBuffers::CreateBuffer(size_t modelCount)
{
	constexpr size_t strideSize = GetStride();

	m_modelBuffersInstanceSize              = static_cast<VkDeviceSize>(strideSize * modelCount);
	const VkDeviceSize modelBufferTotalSize = m_modelBuffersInstanceSize * m_bufferInstanceCount;

	m_buffers.Create(modelBufferTotalSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
}

void ModelBuffers::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot
) const noexcept {
	const auto bufferOffset = static_cast<VkDeviceAddress>(frameIndex * m_modelBuffersInstanceSize);

	descriptorBuffer.SetStorageBufferDescriptor(
		m_buffers, bindingSlot, 0u, bufferOffset, m_modelBuffersInstanceSize
	);
}

void ModelBuffers::_remove(size_t index) noexcept
{
	m_elements.RemoveElement(index, &std::shared_ptr<Model>::reset);
}

void ModelBuffers::Update(VkDeviceSize bufferIndex) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle() + bufferIndex * m_modelBuffersInstanceSize;
	constexpr size_t strideSize = GetStride();
	size_t modelOffset          = 0u;

	auto& models = m_elements.Get();

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

// Model Manager VS Individual
void ModelManagerVSIndividual::CreatePipelineLayoutImpl(const VkDescriptorBuffer& descriptorBuffer)
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = ModelBundleVSIndividual::GetConstantBufferSize();

	m_pipelineLayout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
	m_pipelineLayout.Create(descriptorBuffer);
}

void ModelManagerVSIndividual::ConfigureModel(
	ModelBundleVSIndividual& modelBundleObj, size_t modelIndex, const std::shared_ptr<ModelVS>& model
) {
	modelBundleObj.AddModelDetails(model, static_cast<std::uint32_t>(modelIndex));
}

void ModelManagerVSIndividual::ConfigureModelBundle(
	ModelBundleVSIndividual& modelBundleObj,
	const std::vector<size_t>& modelIndices,
	const std::vector<std::shared_ptr<ModelVS>>& modelBundle
) {
	const size_t modelCount = std::size(modelBundle);

	for (size_t index = 0u; index < modelCount; ++index)
	{
		const std::shared_ptr<ModelVS>& model = modelBundle.at(index);
		const size_t modelIndex               = modelIndices.at(index);

		modelBundleObj.AddModelDetails(model, static_cast<std::uint32_t>(modelIndex));
	}
}

void ModelManagerVSIndividual::ConfigureRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles.at(bundleIndex);

	const auto& modelDetails = modelBundle.GetDetails();

	for (const auto& modelDetail : modelDetails)
		m_modelBuffers.Remove(modelDetail.modelBufferIndex);
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
		modelBundle.Draw(graphicsBuffer, m_pipelineLayout);
	}
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	QueueIndices3 queueIndices3, std::uint32_t frameCount
) : ModelManager{ device, memoryManager, frameCount }, m_stagingBufferMan{ stagingBufferMan },
	m_argumentInputBuffer{
		device, memoryManager,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
	}, m_argumentOutputBuffers{},
	m_cullingDataBuffer{
		device, memoryManager,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, {}
	}, m_counterBuffers{},
	m_counterResetBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
	m_pipelineLayoutCS{ device }, m_computePipeline{}, m_queueIndices3{ queueIndices3 },
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
	m_pipelineLayout.Create(descriptorBuffer);
}

void ModelManagerVSIndirect::CreatePipelineCS(const VkDescriptorBuffer& descriptorBuffer)
{
	m_pipelineLayoutCS.Create(descriptorBuffer);

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

	modelBundleCS.CreateBuffers(*m_stagingBufferMan, m_argumentInputBuffer, m_cullingDataBuffer);

	const auto index32_t = static_cast<std::uint32_t>(modelIndex);

	modelBundleCS.SetID(index32_t);

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	modelBundleObj.AddModelDetails(index32_t);

	modelBundleObj.CreateBuffers(
		*m_stagingBufferMan, m_argumentOutputBuffers, m_counterBuffers
	);

	UpdateCounterResetValues();

	++m_argumentCount;

	UpdateDispatchX();
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
		*m_stagingBufferMan, m_argumentOutputBuffers, m_counterBuffers
	);

	UpdateCounterResetValues();

	modelBundleCS.CreateBuffers(*m_stagingBufferMan, m_argumentInputBuffer, m_cullingDataBuffer);

	m_modelBundlesCS.emplace_back(std::move(modelBundleCS));

	m_argumentCount += static_cast<std::uint32_t>(modelCount);

	UpdateDispatchX();
}

void ModelManagerVSIndirect::ConfigureRemove(size_t bundleIndex) noexcept
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
	}

	std::erase_if(
		m_modelBundlesCS,
		[bundleID, &argumentInput = m_argumentInputBuffer, &cullingData = m_cullingDataBuffer]
		(const ModelBundleCSIndirect& bundle)
		{
			const bool result = bundleID == bundle.GetID();

			if (result)
			{
				argumentInput.RelinquishMemory(bundle.GetArgumentInputSharedData());
				cullingData.RelinquishMemory(bundle.GetCullingSharedData());
			}

			return result;
		}
	);
}

void ModelManagerVSIndirect::_cleanUpTempData() noexcept
{
	// This will be cleaning all models. Might need to do something like a queue.
	for (size_t index = 0u; index < std::size(m_modelBundlesCS); ++index)
	{
		m_modelBundlesCS.at(index).CleanupTempData();
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutVS(
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
		// Argument and counter here.
		descriptorBuffer.AddBinding(
			s_modelIndicesVSBindingSlot, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_VERTEX_BIT
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers.at(index);
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot
		);

		// This is wrong and needs to be updated.
		for (auto& modelBundle : m_modelBundles)
			modelBundle.SetDescriptorBuffer(descriptorBuffer, 2u);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) {
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
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers
) {
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
	}
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

void ModelManagerVSIndirect::CopyTempBuffers(VKCommandBuffer& transferBuffer) const noexcept
{
	m_argumentInputBuffer.CopyOldBuffer(transferBuffer);
	m_cullingDataBuffer.CopyOldBuffer(transferBuffer);

	for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
	{
		m_argumentOutputBuffers.at(index).CopyOldBuffer(transferBuffer);
		m_counterBuffers.at(index).CopyOldBuffer(transferBuffer);
	}
}

void ModelManagerVSIndirect::ResetCounterBuffer(
	VKCommandBuffer& transferBuffer, VkDeviceSize frameIndex
) const noexcept {
	transferBuffer.CopyWhole(m_counterResetBuffer, m_counterBuffers.at(frameIndex).GetBuffer());
}

void ModelManagerVSIndirect::UpdateCounterResetValues()
{
	if (!std::empty(m_counterBuffers))
	{
		const SharedBuffer& counterBuffer    = m_counterBuffers.front();

		const VkDeviceSize counterBufferSize = counterBuffer.Size();
		const VkDeviceSize oldCounterSize    = m_counterResetBuffer.Size();

		if (counterBufferSize > oldCounterSize)
		{
			const size_t counterSize = sizeof(std::uint32_t);

			m_counterResetBuffer.Create(counterBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {});

			constexpr std::uint32_t value = 0u;

			size_t offset             = 0u;
			std::uint8_t* bufferStart = m_counterResetBuffer.CPUHandle();

			for (; offset < counterBufferSize; offset += counterSize)
				memcpy(bufferStart + offset, &value, counterSize);
		}
	}
}
