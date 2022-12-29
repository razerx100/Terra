#include <ranges>
#include <algorithm>

#include <BufferManager.hpp>
#include <Terra.hpp>

BufferManager::BufferManager(Args& arguments)
	: m_cameraBuffer{ arguments.device.value() }, m_modelBuffers{ arguments.device.value() },
	m_bufferCount{ arguments.bufferCount.value() },
	m_computeAndGraphicsQueueIndices{
		std::move(arguments.computeAndGraphicsQueueIndices.value())
	} {}

void BufferManager::CreateBuffers(VkDevice device) noexcept {
	static constexpr size_t cameraBufferSize = sizeof(DirectX::XMMATRIX) * 2u;

	m_cameraBuffer.CreateResource(
		device, cameraBufferSize, m_bufferCount, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		m_computeAndGraphicsQueueIndices
	);

	m_cameraBuffer.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	DescriptorInfo cameraDescInfo{
		.bindingSlot = 0u,
		.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
	};
	auto cameraBufferInfos = m_cameraBuffer.GetDescBufferInfoSplit(m_bufferCount);

	Terra::graphicsDescriptorSet->AddBuffersSplit(
		cameraDescInfo, cameraBufferInfos, VK_SHADER_STAGE_VERTEX_BIT
	);
	Terra::computeDescriptorSet->AddBuffersSplit(
		cameraDescInfo, std::move(cameraBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);

	const size_t modelCount = std::size(m_opaqueModels);
	const size_t modelBufferSize = sizeof(ModelConstantBuffer) * modelCount;

	m_modelBuffers.CreateResource(
		device, static_cast<VkDeviceSize>(modelBufferSize), m_bufferCount,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_computeAndGraphicsQueueIndices
	);
	m_modelBuffers.SetMemoryOffsetAndType(device, MemoryType::cpuWrite);

	DescriptorInfo modelDescInfo{
		.bindingSlot = 2u,
		.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
	};
	auto modelBufferInfos = m_modelBuffers.GetDescBufferInfoSplit(m_bufferCount);

	Terra::graphicsDescriptorSet->AddBuffersSplit(
		modelDescInfo, modelBufferInfos, VK_SHADER_STAGE_VERTEX_BIT
	);

	modelDescInfo.bindingSlot = 1u;
	Terra::computeDescriptorSet->AddBuffersSplit(
		modelDescInfo, std::move(modelBufferInfos), VK_SHADER_STAGE_COMPUTE_BIT
	);
}

void BufferManager::AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept {
	std::ranges::move(models, std::back_inserter(m_opaqueModels));
}

void BufferManager::Update(VkDeviceSize index) const noexcept {
	std::uint8_t* cpuMemoryStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();

	Terra::cameraManager->CopyData(cpuMemoryStart + m_cameraBuffer.GetMemoryOffset(index));

	UpdateModelData(index);
}

void BufferManager::UpdateModelData(VkDeviceSize index) const noexcept {
	size_t offset = 0u;
	constexpr size_t bufferStride = sizeof(ModelConstantBuffer);

	std::uint8_t* cpuWriteStart = Terra::Resources::cpuWriteMemory->GetMappedCPUPtr();
	const VkDeviceSize modelBuffersOffset = m_modelBuffers.GetMemoryOffset(index);

	for (auto& model : m_opaqueModels) {
		ModelConstantBuffer modelBuffer{};
		modelBuffer.textureIndex = model->GetTextureIndex();
		modelBuffer.uvInfo = model->GetUVInfo();
		modelBuffer.modelMatrix = model->GetModelMatrix();
		modelBuffer.modelOffset = model->GetModelOffset();

		const auto& boundingBox = model->GetBoundingBox();
		modelBuffer.positiveBounds = boundingBox.positiveAxes;
		modelBuffer.negativeBounds = boundingBox.negativeAxes;

		// Copy Model Buffer
		memcpy(cpuWriteStart + modelBuffersOffset + offset, &modelBuffer, bufferStride);

		offset += bufferStride;
	}
}

void BufferManager::BindResourceToMemory(VkDevice device) const noexcept {
	m_modelBuffers.BindResourceToMemory(device);
	m_cameraBuffer.BindResourceToMemory(device);
}
