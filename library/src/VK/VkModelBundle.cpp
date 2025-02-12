#include <VkModelBundle.hpp>

// Model Bundle
VkDrawIndexedIndirectCommand ModelBundleBase::GetDrawIndexedIndirectCommand(
	const MeshTemporaryDetailsVS& meshDetailsVS
) noexcept {
	const VkDrawIndexedIndirectCommand indirectCommand{
		.indexCount    = meshDetailsVS.indexCount,
		.instanceCount = 1u,
		.firstIndex    = meshDetailsVS.indexOffset,
		.vertexOffset  = 0,
		.firstInstance = 0u
	};

	return indirectCommand;
}

// Model Bundle VS Individual
void ModelBundleVSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBufferIndices = std::move(modelBufferIndices);
	m_modelBundle        = std::move(bundle);
}

void ModelBundleVSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
	const VkMeshBundleVS& meshBundle
) const noexcept {
	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();
	const auto& models        = m_modelBundle->GetModels();

	for(size_t index = 0; index < std::size(models); ++index)
	{
		const std::shared_ptr<Model>& model = models[index];

		if (!model->IsVisible())
			continue;

		constexpr auto pushConstantSize = GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
			pushConstantSize, &m_modelBufferIndices[index]
		);

		const MeshTemporaryDetailsVS& meshDetailsVS = meshBundle.GetMeshDetails(model->GetMeshIndex());
		const VkDrawIndexedIndirectCommand meshArgs = GetDrawIndexedIndirectCommand(meshDetailsVS);

		vkCmdDrawIndexed(
			cmdBuffer, meshArgs.indexCount, meshArgs.instanceCount,
			meshArgs.firstIndex, meshArgs.vertexOffset, meshArgs.firstInstance
		);
	}
}

// Model Bundle MS
void ModelBundleMSIndividual::SetModelBundle(
	std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBundle        = std::move(bundle);
	m_modelBufferIndices = std::move(modelBufferIndices);
}

void ModelBundleMSIndividual::Draw(
	const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout,
	const VkMeshBundleMS& meshBundle
) const noexcept {
	using MS = VkDeviceExtension::VkExtMeshShader;

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();
	const auto& models        = m_modelBundle->GetModels();

	for (size_t index = 0u; index < std::size(models); ++index)
	{
		const std::shared_ptr<Model>& model = models[index];

		if (!model->IsVisible())
			continue;

		constexpr auto pushConstantSize = GetConstantBufferSize();

		const MeshTemporaryDetailsMS& meshDetailsMS = meshBundle.GetMeshDetails(model->GetMeshIndex());

		const ModelDetails modelConstants
		{
			.meshDetails = MeshDetails
				{
					.meshletCount  = meshDetailsMS.meshletCount,
					.meshletOffset = meshDetailsMS.meshletOffset,
					.indexOffset   = meshDetailsMS.indexOffset,
					.primOffset    = meshDetailsMS.primitiveOffset,
					.vertexOffset  = meshDetailsMS.vertexOffset
				},
			.modelBufferIndex = m_modelBufferIndices[index]
		};

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
			0u, pushConstantSize, &modelConstants
		);

		// If we have a Task shader, this will launch a Task global workGroup. We would want
		// each Task shader invocation to process a meshlet and launch the necessary
		// Mesh Shader workGroups. On Nvdia we can have a maximum of 32 invocations active
		// in a subGroup and 64 on AMD. So, a workGroup will be able to work on 32/64
		// meshlets concurrently.
		const std::uint32_t taskGroupCount = DivRoundUp(
			meshDetailsMS.meshletCount, s_taskInvocationCount
		);

		MS::vkCmdDrawMeshTasksEXT(cmdBuffer, taskGroupCount, 1u, 1u);
		// It might be worth checking if we are reaching the Group Count Limit and if needed
		// launch more Groups. Could achieve that by passing a GroupLaunch index.
	}
}

// Model Bundle CS Indirect
ModelBundleCSIndirect::ModelBundleCSIndirect()
	: m_cullingSharedData{ nullptr, 0u, 0u }, m_perModelDataCSSharedData{ nullptr, 0u, 0u },
	m_argumentInputSharedData{}, m_modelBundle{}, m_modelIndices{}
{}

void ModelBundleCSIndirect::ResetCullingData() const noexcept
{
	// Before destroying an object the command count needs to be set to 0,
	// so the model isn't processed in the compute shader anymore.
	std::uint32_t commandCount  = 0u;
	Buffer const* cullingBuffer = m_cullingSharedData.bufferData;

	memcpy(
		cullingBuffer->CPUHandle() + m_cullingSharedData.offset,
		&commandCount, sizeof(std::uint32_t)
	);
}

void ModelBundleCSIndirect::SetModelBundle(
	std::shared_ptr<ModelBundle> bundle, std::vector<std::uint32_t> modelBufferIndices
) noexcept {
	m_modelBundle  = std::move(bundle);
	m_modelIndices = std::move(modelBufferIndices);
}

void ModelBundleCSIndirect::CreateBuffers(
	std::vector<SharedBufferCPU>& argumentInputSharedBuffer,
	SharedBufferCPU& cullingSharedBuffer, SharedBufferCPU& perModelDataCSBuffer
) {
	constexpr size_t argumentStrideSize = sizeof(VkDrawIndexedIndirectCommand);
	constexpr size_t perModelStride     = sizeof(PerModelData);

	const auto argumentCount      = static_cast<std::uint32_t>(std::size(m_modelBundle->GetModels()));
	const auto argumentBufferSize = static_cast<VkDeviceSize>(argumentStrideSize * argumentCount);
	const auto cullingDataSize    = static_cast<VkDeviceSize>(sizeof(CullingData));
	const auto perModelDataSize   = static_cast<VkDeviceSize>(perModelStride * argumentCount);

	{
		const size_t argumentInputBufferCount = std::size(argumentInputSharedBuffer);
		m_argumentInputSharedData.resize(argumentInputBufferCount);

		for (size_t index = 0u; index < argumentInputBufferCount; ++index)
			m_argumentInputSharedData[index] = argumentInputSharedBuffer[index].AllocateAndGetSharedData(
				argumentBufferSize
			);
	}

	// Set the culling data.
	{
		m_cullingSharedData = cullingSharedBuffer.AllocateAndGetSharedData(
			cullingDataSize, true
		);

		CullingData cullingData
		{
			.commandCount  = argumentCount,
			.commandOffset = 0u
		};

		if (!std::empty(m_argumentInputSharedData))
		{
			const SharedBufferData& sharedBufferData = m_argumentInputSharedData.front();

			// The offset on each sharedBuffer should be the same.
			cullingData.commandOffset = static_cast<std::uint32_t>(
				sharedBufferData.offset / argumentStrideSize
			);
		}

		Buffer const* cullingBuffer = m_cullingSharedData.bufferData;

		memcpy(
			cullingBuffer->CPUHandle() + m_cullingSharedData.offset,
			&cullingData, cullingDataSize
		);
	}

	{
		const std::uint32_t modelBundleIndex = GetModelBundleIndex();

		// Each thread will process a single model independently. And since we are trying to
		// cull all of the models across all of the bundles with a single call to dispatch, we can't
		// set the index as constantData per bundle. So, we will be giving each model the index
		// of its bundle so each thread can work independently.

		m_perModelDataCSSharedData = perModelDataCSBuffer.AllocateAndGetSharedData(
			perModelDataSize, true
		);

		std::uint8_t* bufferStart = m_perModelDataCSSharedData.bufferData->CPUHandle();
		auto offset               = static_cast<size_t>(m_perModelDataCSSharedData.offset);

		for (std::uint32_t modelIndex : m_modelIndices)
		{
			PerModelData perModelData
			{
				.bundleIndex = modelBundleIndex,
				.modelIndex  = modelIndex,
				.isVisible   = 1u
			};

			memcpy(bufferStart + offset, &perModelData, perModelStride);

			offset += perModelStride;
		}
	}
}

void ModelBundleCSIndirect::Update(
	size_t bufferIndex, const VkMeshBundleVS& meshBundle
) const noexcept {
	const SharedBufferData& argumentInputSharedData = m_argumentInputSharedData[bufferIndex];

	std::uint8_t* argumentInputStart  = argumentInputSharedData.bufferData->CPUHandle();
	std::uint8_t* perModelBufferStart = m_perModelDataCSSharedData.bufferData->CPUHandle();

	constexpr size_t argumentStride = sizeof(VkDrawIndexedIndirectCommand);
	auto argumentOffset             = static_cast<size_t>(argumentInputSharedData.offset);

	constexpr size_t perModelStride = sizeof(PerModelData);
	auto perModelOffset             = static_cast<size_t>(m_perModelDataCSSharedData.offset);
	constexpr auto isVisibleOffset  = offsetof(PerModelData, isVisible);
	constexpr auto modelIndexOffset = offsetof(PerModelData, modelIndex);

	const auto& models              = m_modelBundle->GetModels();
	const size_t modelCount         = std::size(models);

	for (size_t index = 0; index < modelCount; ++index)
	{
		const std::shared_ptr<Model>& model         = models[index];

		const MeshTemporaryDetailsVS& meshDetailsVS = meshBundle.GetMeshDetails(model->GetMeshIndex());
		const VkDrawIndexedIndirectCommand meshArgs = ModelBundleBase::GetDrawIndexedIndirectCommand(
			meshDetailsVS
		);

		memcpy(argumentInputStart + argumentOffset, &meshArgs, argumentStride);

		argumentOffset += argumentStride;

		// Model Visiblity
		const auto visiblity = static_cast<std::uint32_t>(model->IsVisible());

		memcpy(perModelBufferStart + perModelOffset + isVisibleOffset, &visiblity, sizeof(std::uint32_t));

		// Model Index
		memcpy(
			perModelBufferStart + perModelOffset + modelIndexOffset,
			&m_modelIndices[index], sizeof(std::uint32_t)
		);

		perModelOffset += perModelStride;
	}
}

// Model Bundle VS Indirect
ModelBundleVSIndirect::ModelBundleVSIndirect()
	: ModelBundleBase{}, m_modelOffset{ 0u }, m_modelBundle {}, m_argumentOutputSharedData{},
	m_counterSharedData{}, m_modelIndicesSharedData{}, m_modelCount{ 0u }, m_bundleID{ 0u }
{}

void ModelBundleVSIndirect::SetModelBundle(std::shared_ptr<ModelBundle> bundle) noexcept
{
	m_modelBundle = std::move(bundle);
}

void ModelBundleVSIndirect::CreateBuffers(
	std::vector<SharedBufferGPUWriteOnly>& argumentOutputSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& counterSharedBuffers,
	std::vector<SharedBufferGPUWriteOnly>& modelIndicesSharedBuffers,
	std::uint32_t modelCount
) {
	constexpr size_t argStrideSize      = sizeof(VkDrawIndexedIndirectCommand);
	constexpr size_t indexStrideSize    = sizeof(std::uint32_t);
	const auto argumentOutputBufferSize = static_cast<VkDeviceSize>(modelCount * argStrideSize);
	const auto modelIndiceBufferSize    = static_cast<VkDeviceSize>(modelCount * indexStrideSize);

	m_modelCount                        = modelCount;

	{
		const size_t argumentOutputBufferCount = std::size(argumentOutputSharedBuffers);
		m_argumentOutputSharedData.resize(argumentOutputBufferCount);

		for (size_t index = 0u; index < argumentOutputBufferCount; ++index)
		{
			SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[index];

			argumentOutputSharedData = argumentOutputSharedBuffers[index].AllocateAndGetSharedData(
				argumentOutputBufferSize
			);

			// The offset on each sharedBuffer should be the same. But still need to keep track of each
			// of them because we will need the Buffer object to draw.
			m_modelOffset = static_cast<std::uint32_t>(argumentOutputSharedData.offset / argStrideSize);
		}
	}

	{
		const size_t counterBufferCount = std::size(counterSharedBuffers);
		m_counterSharedData.resize(counterBufferCount);

		for (size_t index = 0u; index < counterBufferCount; ++index)
			m_counterSharedData[index] = counterSharedBuffers[index].AllocateAndGetSharedData(
				s_counterBufferSize
			);
	}

	{
		const size_t modelIndicesBufferCount = std::size(modelIndicesSharedBuffers);
		m_modelIndicesSharedData.resize(modelIndicesBufferCount);

		for (size_t index = 0u; index < modelIndicesBufferCount; ++index)
			m_modelIndicesSharedData[index] = modelIndicesSharedBuffers[index].AllocateAndGetSharedData(
				modelIndiceBufferSize
			);
	}
}

void ModelBundleVSIndirect::Draw(
	size_t frameIndex, const VKCommandBuffer& graphicsBuffer, VkPipelineLayout pipelineLayout
) const noexcept {
	constexpr auto strideSize = static_cast<std::uint32_t>(sizeof(VkDrawIndexedIndirectCommand));

	VkCommandBuffer cmdBuffer = graphicsBuffer.Get();

	{
		constexpr auto pushConstantSize = GetConstantBufferSize();

		vkCmdPushConstants(
			cmdBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0u,
			pushConstantSize, &m_modelOffset
		);
	}

	const SharedBufferData& argumentOutputSharedData = m_argumentOutputSharedData[frameIndex];
	const SharedBufferData& counterSharedData        = m_counterSharedData[frameIndex];

	vkCmdDrawIndexedIndirectCount(
		cmdBuffer,
		argumentOutputSharedData.bufferData->Get(), argumentOutputSharedData.offset,
		counterSharedData.bufferData->Get(), counterSharedData.offset,
		GetModelCount(), strideSize
	);
}
