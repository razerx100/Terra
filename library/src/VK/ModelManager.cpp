#include <ModelManager.hpp>
#include <VectorToSharedPtr.hpp>
#include <VkResourceBarriers2.hpp>

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

	const size_t modelCount             = m_elements.GetCount();

	// All of the models will be here. Even after multiple models have been removed, there
	// should be null models there. It is necessary to keep them to preserve the model indices,
	// which is used to keep track of the models both on the CPU and the GPU side.
	for (size_t index = 0u; index < modelCount; ++index)
	{
		// Don't update the data if the model is not in use. Could use this functionality to
		// temporarily hide models later.
		if (m_elements.IsInUse(index))
		{
			const auto& model = m_elements.at(index);

			// Vertex Data
			{
				const ModelVertexData modelVertexData
				{
					.modelMatrix   = model->GetModelMatrix(),
					.modelOffset   = model->GetModelOffset(),
					.materialIndex = model->GetMaterialIndex(),
					.meshIndex     = model->GetMeshIndex(),
					.modelScale    = model->GetModelScale()
				};

				memcpy(vertexBufferOffset + vertexModelOffset, &modelVertexData, vertexStrideSize);
			}

			// Fragment Data
			{
				const ModelFragmentData modelFragmentData
				{
					.diffuseTexUVInfo  = model->GetDiffuseUVInfo(),
					.specularTexUVInfo = model->GetSpecularUVInfo(),
					.diffuseTexIndex   = model->GetDiffuseIndex(),
					.specularTexIndex  = model->GetSpecularIndex()
				};

				memcpy(fragmentBufferOffset + fragmentModelOffset, &modelFragmentData, fragmentStrideSize);
			}
		}
		// The offsets need to be always increased to keep them consistent.
		vertexModelOffset   += vertexStrideSize;
		fragmentModelOffset += fragmentStrideSize;
	}
}

// Model Manager VS Individual
ModelManagerVSIndividual::ModelManagerVSIndividual(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3,
	std::uint32_t frameCount
) : ModelManager{ device, memoryManager, queueIndices3, frameCount },
	m_vertexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}, m_indexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}
{}

void ModelManagerVSIndividual::_setGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t pushConstantSize = ModelBundleVSIndividual::GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
}

void ModelManagerVSIndividual::ConfigureModelBundle(
	ModelBundleVSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle, TemporaryDataBufferGPU&// Not needed in this system.
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
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer,
		tempBuffer
	);
}

void ModelManagerVSIndividual::CopyOldBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_indexBuffer.CopyOldBuffer(transferCmdBuffer);
		m_vertexBuffer.CopyOldBuffer(transferCmdBuffer);

		m_oldBufferCopyNecessary = false;
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
		const MeshManagerVertexShader& meshBundle = m_meshBundles.at(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);
		meshBundle.Bind(graphicsBuffer);

		// Model
		modelBundle.Draw(graphicsBuffer, m_graphicsPipelineLayout, meshBundle);
	}
}

ModelBuffers ModelManagerVSIndividual::ConstructModelBuffers(
	VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount,
	[[maybe_unused]] QueueIndices3 queueIndices
) {
	// Only being accessed from the graphics queue.
	return ModelBuffers{ device, memoryManager, frameCount, {} };
}

// Model Manager VS Indirect.
ModelManagerVSIndirect::ModelManagerVSIndirect(
	VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	QueueIndices3 queueIndices3, std::uint32_t frameCount
) : ModelManager{ device, memoryManager, queueIndices3, frameCount },
	m_stagingBufferMan{ stagingBufferMan }, m_argumentInputBuffers{}, m_argumentOutputBuffers{},
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
	}, m_vertexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}, m_indexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}, m_perMeshDataBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTC>()
	}, m_perMeshBundleDataBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTC>()
	}, m_pipelineLayoutCS{ VK_NULL_HANDLE }, m_computePipeline{},
	m_queueIndices3{ queueIndices3 }, m_dispatchXCount{ 0u }, m_argumentCount{ 0u }, m_modelBundlesCS{}
{
	for (size_t _ = 0u; _ < frameCount; ++_)
	{
		// Only getting written and read on the Compute Queue, so should be exclusive resource.
		m_argumentInputBuffers.emplace_back(
			SharedBufferCPU{ m_device, m_memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {} }
		);
		m_argumentOutputBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				m_device, m_memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
		// Doing the resetting on the Compute queue, so CG should be fine.
		m_counterBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				m_device, m_memoryManager,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
		m_modelIndicesVSBuffers.emplace_back(
			SharedBufferGPUWriteOnly{
				m_device, m_memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				m_queueIndices3.ResolveQueueIndices<QueueIndicesCG>()
			}
		);
	}
}

void ModelManagerVSIndirect::_setGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	constexpr auto pushConstantSize = ModelBundleVSIndirect::GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_VERTEX_BIT, pushConstantSize);
}

void ModelManagerVSIndirect::SetComputePipelineLayout(VkPipelineLayout layout) noexcept
{
	m_pipelineLayoutCS = layout;
}

void ModelManagerVSIndirect::SetComputeConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr auto pushConstantSize = GetConstantBufferSize();

	layout.AddPushConstantRange(VK_SHADER_STAGE_COMPUTE_BIT, pushConstantSize);
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
	std::shared_ptr<ModelBundle>&& modelBundle, TemporaryDataBufferGPU& tempBuffer
) {
	ModelBundleCSIndirect modelBundleCS{};

	modelBundleCS.SetModelBundle(modelBundle);
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));

	modelBundleCS.SetID(static_cast<std::uint32_t>(modelBundleObj.GetID()));

	modelBundleObj.CreateBuffers(m_argumentOutputBuffers, m_counterBuffers, m_modelIndicesVSBuffers);

	UpdateCounterResetValues();

	modelBundleCS.CreateBuffers(
		*m_stagingBufferMan, m_argumentInputBuffers, m_cullingDataBuffer,
		m_perModelDataCSBuffer, modelBundleObj.GetModelIndices(), tempBuffer
	);

	const auto modelBundleIndexInBuffer = modelBundleCS.GetModelBundleIndex();

	m_meshBundleIndexBuffer.Add(modelBundleIndexInBuffer);

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

	const auto bundleID = static_cast<std::uint32_t>(modelBundle.GetID());

	{
		const std::vector<SharedBufferData>& argumentOutputSharedData
			= modelBundle.GetArgumentOutputSharedData();

		for (size_t index = 0u; index < std::size(m_argumentOutputBuffers); ++index)
			m_argumentOutputBuffers[index].RelinquishMemory(argumentOutputSharedData[index]);

		const std::vector<SharedBufferData>& counterSharedData = modelBundle.GetCounterSharedData();

		for (size_t index = 0u; index < std::size(m_counterBuffers); ++index)
			m_counterBuffers[index].RelinquishMemory(counterSharedData[index]);

		const std::vector<SharedBufferData>& modelIndicesSharedData
			= modelBundle.GetModelIndicesSharedData();

		for (size_t index = 0u; index < std::size(m_modelIndicesVSBuffers); ++index)
			m_modelIndicesVSBuffers[index].RelinquishMemory(modelIndicesSharedData[index]);
	}

	std::erase_if(
		m_modelBundlesCS,
		[bundleID, &argumentInputs = m_argumentInputBuffers,
		&cullingData    = m_cullingDataBuffer,
		&perModelDataCS = m_perModelDataCSBuffer]
		(const ModelBundleCSIndirect& bundle)
		{
			const bool result = bundleID == bundle.GetID();

			if (result)
			{
				{
					const std::vector<SharedBufferData>& argumentInputSharedData
						= bundle.GetArgumentInputSharedData();

					for (size_t index = 0u; index < std::size(argumentInputs); ++index)
						argumentInputs[index].RelinquishMemory(argumentInputSharedData[index]);
				}

				bundle.ResetCullingData();

				cullingData.RelinquishMemory(bundle.GetCullingSharedData());
				perModelDataCS.RelinquishMemory(bundle.GetPerModelDataCSSharedData());
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

		const SharedBufferData& perMeshSharedData = meshManager.GetPerMeshSharedData();
		m_perMeshDataBuffer.RelinquishMemory(perMeshSharedData);

		const SharedBufferData& perMeshBundleSharedData = meshManager.GetPerMeshBundleSharedData();
		m_perMeshBundleDataBuffer.RelinquishMemory(perMeshBundleSharedData);
	}
}

void ModelManagerVSIndirect::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerVertexShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, m_perMeshDataBuffer,
		m_perMeshBundleDataBuffer, tempBuffer
	);
}

void ModelManagerVSIndirect::_updatePerFrame(VkDeviceSize frameIndex) const noexcept
{
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

		const MeshManagerVertexShader& meshBundle = m_meshBundles.at(
			static_cast<size_t>(meshBundleIndex)
		);

		bundle.Update(static_cast<size_t>(frameIndex), meshBundle);

		const std::uint32_t modelBundleIndex = bundle.GetModelBundleIndex();

		bufferOffset = strideSize * modelBundleIndex;

		memcpy(bufferOffsetPtr + bufferOffset, &meshBundleIndex, strideSize);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutVS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t vsSetLayoutIndex, size_t fsSetLayoutIndex
) const noexcept {
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
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];
		const auto frameIndex                = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot, vsSetLayoutIndex
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_modelIndicesVSBuffers[index].GetBuffer(), s_modelIndicesVSBindingSlot, vsSetLayoutIndex, 0u
		);
	}
}

void ModelManagerVSIndirect::SetDescriptorBufferLayoutCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];

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
			s_perModelDataCSBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_perMeshDataBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_perMeshBundleDataBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
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

void ModelManagerVSIndirect::SetDescriptorBufferCSOfModels(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];
		const auto frameIndex = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersComputeBindingSlot, csSetLayoutIndex
		);

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

void ModelManagerVSIndirect::SetDescriptorBufferCSOfMeshes(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const {
	for (auto& descriptorBuffer : descriptorBuffers)
	{
		descriptorBuffer.SetStorageBufferDescriptor(
			m_perMeshDataBuffer.GetBuffer(), s_perMeshDataBindingSlot, csSetLayoutIndex, 0u
		);
		descriptorBuffer.SetStorageBufferDescriptor(
			m_perMeshBundleDataBuffer.GetBuffer(), s_perMeshBundleDataBindingSlot, csSetLayoutIndex,
			0u
		);
	}
}

void ModelManagerVSIndirect::Dispatch(const VKCommandBuffer& computeBuffer) const noexcept
{
	VkCommandBuffer cmdBuffer = computeBuffer.Get();

	m_computePipeline.Bind(computeBuffer);

	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		const ConstantData constantData
		{
			.modelCount = m_argumentCount
		};

		vkCmdPushConstants(
			cmdBuffer, m_pipelineLayoutCS, VK_SHADER_STAGE_COMPUTE_BIT, 0u,
			pushConstantSize, &constantData
		);
	}

	vkCmdDispatch(cmdBuffer, m_dispatchXCount, 1u, 1u);
}

void ModelManagerVSIndirect::Draw(size_t frameIndex, const VKCommandBuffer& graphicsBuffer) const noexcept
{
	auto previousPSOIndex = std::numeric_limits<size_t>::max();

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, previousPSOIndex);

		// Mesh
		const MeshManagerVertexShader& meshBundle = m_meshBundles.at(
			static_cast<size_t>(modelBundle.GetMeshBundleIndex())
		);
		meshBundle.Bind(graphicsBuffer);

		// Model
		modelBundle.Draw(frameIndex, graphicsBuffer, m_graphicsPipelineLayout);
	}
}

void ModelManagerVSIndirect::CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_perModelDataCSBuffer.CopyOldBuffer(transferBuffer);
		m_vertexBuffer.CopyOldBuffer(transferBuffer);
		m_indexBuffer.CopyOldBuffer(transferBuffer);
		m_perMeshDataBuffer.CopyOldBuffer(transferBuffer);
		m_perMeshBundleDataBuffer.CopyOldBuffer(transferBuffer);

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

ModelBuffers ModelManagerVSIndirect::ConstructModelBuffers(
	VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount, QueueIndices3 queueIndices
) {
	// Will be accessed from both the Graphics queue and the compute queue.
	return ModelBuffers{
		device, memoryManager, frameCount, queueIndices.ResolveQueueIndices<QueueIndicesCG>()
	};
}

// Model Manager MS.
ModelManagerMS::ModelManagerMS(
	VkDevice device, MemoryManager* memoryManager, StagingBufferManager* stagingBufferMan,
	QueueIndices3 queueIndices3, std::uint32_t frameCount
) : ModelManager{ device, memoryManager, queueIndices3, frameCount },
	m_stagingBufferMan{ stagingBufferMan },
	m_perMeshletDataBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}, m_vertexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}, m_vertexIndicesBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}, m_primIndicesBuffer{
		device, memoryManager, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}
{}

void ModelManagerMS::ConfigureModelBundle(
	ModelBundleMSIndividual& modelBundleObj, std::vector<std::uint32_t>&& modelIndices,
	std::shared_ptr<ModelBundle>&& modelBundle, [[maybe_unused]] TemporaryDataBufferGPU& tempBuffer
) {
	modelBundleObj.SetModelBundle(std::move(modelBundle), std::move(modelIndices));
}

void ModelManagerMS::ConfigureModelRemove(size_t bundleIndex) noexcept
{
	const auto& modelBundle  = m_modelBundles[bundleIndex];

	const auto& modelIndices = modelBundle.GetIndices();

	for (std::uint32_t modelIndex : modelIndices)
		m_modelBuffers.Remove(modelIndex);
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

		const SharedBufferData& perMeshletSharedData    = meshManager.GetPerMeshletSharedData();
		m_perMeshletDataBuffer.RelinquishMemory(perMeshletSharedData);
	}
}

void ModelManagerMS::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	MeshManagerMeshShader& meshManager, TemporaryDataBufferGPU& tempBuffer
) {
	meshManager.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_vertexIndicesBuffer,
		m_primIndicesBuffer, m_perMeshletDataBuffer, tempBuffer
	);
}

void ModelManagerMS::_setGraphicsConstantRange(PipelineLayout& layout) noexcept
{
	// Push constants needs to be serialised according to the shader stages
	constexpr std::uint32_t modelConstantSize = ModelBundleMSIndividual::GetConstantBufferSize();
	constexpr std::uint32_t meshConstantSize  = MeshManagerMeshShader::GetConstantBufferSize();

	layout.AddPushConstantRange(
		VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
		modelConstantSize + meshConstantSize
	);
}

void ModelManagerMS::CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_perMeshletDataBuffer.CopyOldBuffer(transferBuffer);
		m_vertexBuffer.CopyOldBuffer(transferBuffer);
		m_vertexIndicesBuffer.CopyOldBuffer(transferBuffer);
		m_primIndicesBuffer.CopyOldBuffer(transferBuffer);

		m_oldBufferCopyNecessary = false;
	}
}

void ModelManagerMS::SetDescriptorBufferLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex, size_t fsSetLayoutIndex
) const noexcept {
	const auto frameCount = std::size(descriptorBuffers);

	for (size_t index = 0u; index < frameCount; ++index)
	{
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];

		descriptorBuffer.AddBinding(
			s_modelBuffersGraphicsBindingSlot, msSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT
		);
		descriptorBuffer.AddBinding(
			s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descriptorBuffer.AddBinding(
			s_perMeshletBufferBindingSlot, msSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT
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
		VkDescriptorBuffer& descriptorBuffer = descriptorBuffers[index];
		const auto frameIndex = static_cast<VkDeviceSize>(index);

		m_modelBuffers.SetDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersGraphicsBindingSlot, msSetLayoutIndex
		);
		m_modelBuffers.SetFragmentDescriptorBuffer(
			descriptorBuffer, frameIndex, s_modelBuffersFragmentBindingSlot, fsSetLayoutIndex
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
		descriptorBuffer.SetStorageBufferDescriptor(
			m_perMeshletDataBuffer.GetBuffer(), s_perMeshletBufferBindingSlot, msSetLayoutIndex, 0u
		);
	}
}

void ModelManagerMS::Draw(const VKCommandBuffer& graphicsBuffer) const noexcept
{
	auto previousPSOIndex     = std::numeric_limits<size_t>::max();

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	for (const auto& modelBundle : m_modelBundles)
	{
		// Pipeline Object.
		BindPipeline(modelBundle, graphicsBuffer, previousPSOIndex);

		const auto meshBundleIndex = static_cast<size_t>(
			modelBundle.GetMeshBundleIndex()
		);
		const MeshManagerMeshShader& meshBundle = m_meshBundles.at(meshBundleIndex);

		constexpr std::uint32_t constantBufferOffset
			= ModelBundleMSIndividual::GetConstantBufferSize();

		constexpr std::uint32_t constBufferSize = MeshManagerMeshShader::GetConstantBufferSize();

		const MeshManagerMeshShader::MeshBundleDetailsMS meshBundleDetailsMS
			= meshBundle.GetMeshBundleDetailsMS();

		vkCmdPushConstants(
			cmdBuffer, m_graphicsPipelineLayout,
			VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
			constantBufferOffset, constBufferSize, &meshBundleDetailsMS
		);

		// Model
		modelBundle.Draw(graphicsBuffer, m_graphicsPipelineLayout, meshBundle);
	}
}

ModelBuffers ModelManagerMS::ConstructModelBuffers(
	VkDevice device, MemoryManager* memoryManager, std::uint32_t frameCount,
	[[maybe_unused]] QueueIndices3 queueIndices
) {
	// Will be accessed from both the Graphics queue only.
	return ModelBuffers{ device, memoryManager, frameCount, {} };
}
