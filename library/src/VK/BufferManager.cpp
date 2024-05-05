#include <ranges>
#include <algorithm>

#include <BufferManager.hpp>
#include <Terra.hpp>

BufferManager::BufferManager(
	VkDevice device, std::uint32_t bufferCount, QueueIndicesCG queueIndices, bool modelDataNoBB,
	bool meshShader
) : m_cameraBuffer{ device }, m_modelBuffers{ device }, m_materialBuffers{ device },
	m_lightBuffers{ device }, m_fragmentDataBuffer{ device }, m_bufferCount{ bufferCount },
	m_queueIndices{ queueIndices }, m_modelDataNoBB{ modelDataNoBB }, m_meshShader{ meshShader }
{}

void BufferManager::CreateBuffers(VkDevice device) noexcept {
	// Camera
	static constexpr size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	const VkShaderStageFlagBits vertexType
		= m_meshShader ? VK_SHADER_STAGE_MESH_BIT_EXT : VK_SHADER_STAGE_VERTEX_BIT;

	std::vector<std::uint32_t> resolvedIndices = m_queueIndices.ResolveQueueIndices();

	CreateBufferComputeAndVertex(
		device, m_cameraBuffer, static_cast<VkDeviceSize>(cameraBufferSize),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		{ .bindingSlot = 4u, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }, resolvedIndices,
		vertexType
	);

	// Model
	const size_t modelCount = std::size(m_opaqueModels);
	const size_t modelBufferStride =
		m_modelDataNoBB ? sizeof(ModelBufferNoBB) : sizeof(ModelBuffer);
	const size_t modelBufferSize = modelBufferStride * modelCount;

	CreateBufferComputeAndVertex(
		device, m_modelBuffers, static_cast<VkDeviceSize>(modelBufferSize),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		{ .bindingSlot = 5u, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }, resolvedIndices,
		vertexType
	);

	// Material
	const size_t materialBufferSize = sizeof(MaterialBuffer) * modelCount;

	CreateBufferFragment(
		device, m_materialBuffers, static_cast<VkDeviceSize>(materialBufferSize),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		{ .bindingSlot = 1u, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
	);

	// Light
	const size_t lightBufferSize = sizeof(LightBuffer) * std::size(m_lightModelIndices);

	CreateBufferFragment(
		device, m_lightBuffers, static_cast<VkDeviceSize>(lightBufferSize),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		{ .bindingSlot = 2u, .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
	);

	// Fragment Data
	static constexpr size_t fragmentDataBufferSize = sizeof(FragmentData);

	CreateBufferFragment(
		device, m_fragmentDataBuffer, static_cast<VkDeviceSize>(fragmentDataBufferSize),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		{ .bindingSlot = 3u, .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
	);
}

void BufferManager::CheckLightSourceAndAddOpaque(std::shared_ptr<Model>&& model) noexcept {
	if (model->IsLightSource())
		m_lightModelIndices.emplace_back(std::size(m_opaqueModels));

	m_opaqueModels.emplace_back(std::move(model));
}

void BufferManager::AddOpaqueModels(std::vector<std::shared_ptr<Model>>&& models) noexcept {
	for (size_t index = 0u; index < std::size(models); ++index)
		CheckLightSourceAndAddOpaque(std::move(models[index]));
}

/*void BufferManager::AddOpaqueModels(std::vector<MeshletModel>&& meshletModels) noexcept {
	for (size_t index = 0u; index < std::size(meshletModels); ++index)
		CheckLightSourceAndAddOpaque(std::move(meshletModels[index].model));
}*/

void BufferManager::UpdateCameraData(
	VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart
) const noexcept {
	Terra::Get().Camera().CopyData(cpuMemoryStart + m_cameraBuffer.GetMemoryOffset(bufferIndex));
}

void BufferManager::UpdateLightData(
	VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart, const DirectX::XMMATRIX& viewMatrix
) const noexcept {
	size_t offset = 0u;

	std::uint8_t* lightBuffersOffset =
		cpuMemoryStart + m_lightBuffers.GetMemoryOffset(bufferIndex);

	for (auto& lightIndex : m_lightModelIndices) {
		auto& model = m_opaqueModels[lightIndex];
		/*const auto& modelMaterial = model->GetMaterial();

		LightBuffer light {
			.ambient = modelMaterial.ambient,
			.diffuse = modelMaterial.diffuse,
			.specular = modelMaterial.specular
		};*/

		DirectX::XMFLOAT3 modelPosition = model->GetModelOffset();
		DirectX::XMMATRIX viewSpace = model->GetModelMatrix() * viewMatrix;
		DirectX::XMVECTOR viewPosition = DirectX::XMVector3Transform(
			DirectX::XMLoadFloat3(&modelPosition), viewMatrix
		);
		//DirectX::XMStoreFloat3(&light.position, viewPosition);

		//CopyStruct(light, lightBuffersOffset, offset);
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

void BufferManager::CreateBufferComputeAndVertex(
	VkDevice device, VkResourceView& buffer, VkDeviceSize bufferSize,
	VkBufferUsageFlagBits bufferType, const DescriptorInfo& descInfo,
	const std::vector<std::uint32_t>& resolvedQueueIndices,
	VkShaderStageFlagBits vertexShaderType
) const noexcept {
	buffer.CreateResource(device, bufferSize, m_bufferCount, bufferType, resolvedQueueIndices);
	buffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	auto bufferInfos = buffer.GetDescBufferInfoSplit(m_bufferCount);

	Terra::Get().GraphicsDesc().AddBuffersSplit(descInfo, bufferInfos, vertexShaderType);
	Terra::Get().ComputeDesc().AddBuffersSplit(
		descInfo, std::move(bufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);
}

void BufferManager::CreateBufferFragment(
	VkDevice device, VkResourceView& buffer, VkDeviceSize bufferSize,
	VkBufferUsageFlagBits bufferType, const DescriptorInfo& descInfo
) const noexcept {
	buffer.CreateResource(device, bufferSize, m_bufferCount, bufferType);
	buffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	auto bufferInfos = buffer.GetDescBufferInfoSplit(m_bufferCount);

	Terra::Get().GraphicsDesc().AddBuffersSplit(
		descInfo, bufferInfos, VK_SHADER_STAGE_FRAGMENT_BIT
	);
}

/*DirectX::XMMATRIX BufferManager::GetViewMatrix() const noexcept {
	return m_sharedData.GetViewMatrix();
}*/

std::uint8_t* BufferManager::GetCPUWriteStartMemory() const noexcept {
	return Terra::Get().Res().CPU().GetMappedCPUPtr();
}
