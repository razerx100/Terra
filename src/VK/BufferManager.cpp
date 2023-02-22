#include <ranges>
#include <algorithm>

#include <BufferManager.hpp>
#include <Terra.hpp>

BufferManager::BufferManager(Args& arguments)
	: m_cameraBuffer{ arguments.device.value() }, m_modelBuffers{ arguments.device.value() },
	m_materialBuffers{ arguments.device.value() }, m_lightBuffers{ arguments.device.value() },
	m_fragmentDataBuffer{ arguments.device.value() },
	m_bufferCount{ arguments.bufferCount.value() },
	m_queueIndices{ arguments.queueIndices.value() } {}

void BufferManager::CreateBuffers(VkDevice device) noexcept {
	// Camera
	static constexpr size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	auto resolvedIndices = ResolveQueueIndices(m_queueIndices.compute, m_queueIndices.graphics);

	CreateBufferComputeAndGraphics(
		device, m_cameraBuffer, static_cast<VkDeviceSize>(cameraBufferSize),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		{ .bindingSlot = 0u, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }, resolvedIndices
	);

	// Model
	const size_t modelCount = std::size(m_opaqueModels);
	const size_t modelBufferSize = sizeof(ModelBuffer) * modelCount;

	CreateBufferComputeAndGraphics(
		device, m_modelBuffers, static_cast<VkDeviceSize>(modelBufferSize),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		{ .bindingSlot = 1u, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, resolvedIndices
	);

	// Material
	const size_t materialBufferSize = sizeof(MaterialBuffer) * modelCount;

	CreateBufferGraphics(
		device, m_materialBuffers, static_cast<VkDeviceSize>(materialBufferSize),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		{ .bindingSlot = 3u, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
	);

	// Light
	const size_t lightBufferSize = sizeof(LightBuffer) * std::size(m_lightModelIndices);

	CreateBufferGraphics(
		device, m_lightBuffers, static_cast<VkDeviceSize>(lightBufferSize),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		{ .bindingSlot = 4u, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
	);

	// Fragment Data
	static constexpr size_t fragmentDataBufferSize = sizeof(FragmentData);

	CreateBufferGraphics(
		device, m_fragmentDataBuffer, static_cast<VkDeviceSize>(fragmentDataBufferSize),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		{ .bindingSlot = 5u, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
	);
}

void BufferManager::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	for (size_t index = 0u; index < std::size(models); ++index)
		if (models[index]->IsLightSource())
			m_lightModelIndices.emplace_back(std::size(m_opaqueModels) + index);

	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void BufferManager::Update(VkDeviceSize bufferIndex) const noexcept {
	std::uint8_t* cpuMemoryStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();
	const DirectX::XMMATRIX viewMatrix = Terra::sharedData->GetViewMatrix();

	UpdateCameraData(bufferIndex, cpuMemoryStart);
	UpdatePerModelData(bufferIndex, cpuMemoryStart, viewMatrix);
	UpdateLightData(bufferIndex, cpuMemoryStart, viewMatrix);
	UpdateFragmentData(bufferIndex, cpuMemoryStart);
}

void BufferManager::UpdateCameraData(
	VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart
) const noexcept {
	Terra::cameraManager->CopyData(
		cpuMemoryStart + m_cameraBuffer.GetMemoryOffset(bufferIndex)
	);
}

void BufferManager::UpdatePerModelData(
	VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart, const DirectX::XMMATRIX& viewMatrix
) const noexcept {
	size_t modelOffset = 0u;
	size_t materialOffset = 0u;

	std::uint8_t* modelBuffersOffset =
		cpuMemoryStart + m_modelBuffers.GetMemoryOffset(bufferIndex);
	std::uint8_t* materialBuffersOffset =
		cpuMemoryStart + m_materialBuffers.GetMemoryOffset(bufferIndex);

	for (auto& model : m_opaqueModels) {
		const auto& boundingBox = model->GetBoundingBox();
		const DirectX::XMMATRIX modelMatrix = model->GetModelMatrix();

		ModelBuffer modelBuffer{
			.modelMatrix = modelMatrix,
			.viewNormalMatrix = DirectX::XMMatrixTranspose(
				DirectX::XMMatrixInverse(nullptr, modelMatrix * viewMatrix)
			),
			.modelOffset = model->GetModelOffset(),
			.positiveBounds = boundingBox.positiveAxes,
			.negativeBounds = boundingBox.negativeAxes
		};
		CopyStruct(modelBuffer, modelBuffersOffset, modelOffset);

		const auto& modelMaterial = model->GetMaterial();

		MaterialBuffer material{
			.ambient = modelMaterial.ambient,
			.diffuse = modelMaterial.diffuse,
			.specular = modelMaterial.specular,
			.diffuseTexUVInfo = model->GetDiffuseTexUVInfo(),
			.specularTexUVInfo = model->GetSpecularTexUVInfo(),
			.diffuseTexIndex = model->GetDiffuseTexIndex(),
			.specularTexIndex = model->GetSpecularTexIndex(),
			.shininess = modelMaterial.shininess
		};
		CopyStruct(material, materialBuffersOffset, materialOffset);
	}
}

void BufferManager::UpdateLightData(
	VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart, const DirectX::XMMATRIX& viewMatrix
) const noexcept {
	size_t offset = 0u;

	std::uint8_t* lightBuffersOffset =
		cpuMemoryStart + m_lightBuffers.GetMemoryOffset(bufferIndex);

	for (auto& lightIndex : m_lightModelIndices) {
		auto& model = m_opaqueModels[lightIndex];
		const auto& modelMaterial = model->GetMaterial();

		LightBuffer light {
			.ambient = modelMaterial.ambient,
			.diffuse = modelMaterial.diffuse,
			.specular = modelMaterial.specular
		};

		DirectX::XMFLOAT3 modelPosition = model->GetModelOffset();
		DirectX::XMMATRIX viewSpace = model->GetModelMatrix() * viewMatrix;
		DirectX::XMVECTOR viewPosition = DirectX::XMVector3Transform(
			DirectX::XMLoadFloat3(&modelPosition), viewMatrix
		);
		DirectX::XMStoreFloat3(&light.position, viewPosition);

		CopyStruct(light, lightBuffersOffset, offset);
	}
}

void BufferManager::UpdateFragmentData(
	VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart
) const noexcept {
	const VkDeviceSize fragmentDataOffset = m_fragmentDataBuffer.GetMemoryOffset(bufferIndex);
	const auto lightCount = static_cast<std::uint32_t>(std::size(m_lightModelIndices));

	memcpy(cpuMemoryStart + fragmentDataOffset, &lightCount, sizeof(FragmentData));
}

void BufferManager::BindResourceToMemory(VkDevice device) const noexcept {
	m_modelBuffers.BindResourceToMemory(device);
	m_cameraBuffer.BindResourceToMemory(device);
	m_materialBuffers.BindResourceToMemory(device);
	m_lightBuffers.BindResourceToMemory(device);
	m_fragmentDataBuffer.BindResourceToMemory(device);
}

void BufferManager::CreateBufferComputeAndGraphics(
	VkDevice device, VkResourceView& buffer, VkDeviceSize bufferSize,
	VkBufferUsageFlagBits bufferType, const DescriptorInfo& descInfo,
	const std::vector<std::uint32_t>& resolvedQueueIndices
) const noexcept {
	buffer.CreateResource(device, bufferSize, m_bufferCount, bufferType, resolvedQueueIndices);
	buffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	auto bufferInfos = buffer.GetDescBufferInfoSplit(m_bufferCount);

	Terra::graphicsDescriptorSet->AddBuffersSplit(
		descInfo, bufferInfos, VK_SHADER_STAGE_VERTEX_BIT
	);
	Terra::computeDescriptorSet->AddBuffersSplit(
		descInfo, std::move(bufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);
}

void BufferManager::CreateBufferGraphics(
	VkDevice device, VkResourceView& buffer, VkDeviceSize bufferSize,
	VkBufferUsageFlagBits bufferType, const DescriptorInfo& descInfo
) const noexcept {
	buffer.CreateResource(device, bufferSize, m_bufferCount, bufferType);
	buffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	auto bufferInfos = buffer.GetDescBufferInfoSplit(m_bufferCount);

	Terra::graphicsDescriptorSet->AddBuffersSplit(
		descInfo, bufferInfos, VK_SHADER_STAGE_FRAGMENT_BIT
	);
}
