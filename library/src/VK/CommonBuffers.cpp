#include <CommonBuffers.hpp>

// Material Buffers
void MaterialBuffers::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
) const noexcept {
	descriptorBuffer.SetStorageBufferDescriptor(m_buffers, bindingSlot, 0u);
}

void MaterialBuffers::CreateBuffer(size_t materialCount)
{
	constexpr size_t strideSize    = GetStride();
	const auto materialBuffersSize = static_cast<VkDeviceSize>(strideSize * materialCount);

	m_buffers.Create(materialBuffersSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
}

void MaterialBuffers::_remove(size_t index) noexcept
{
	m_elements.RemoveElement(index, &std::shared_ptr<Material>::reset);
}

void MaterialBuffers::Update(size_t index) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();
	size_t materialOffset       = index * strideSize;

	auto& materials = m_elements.Get();

	const std::shared_ptr<Material>& material = materials.at(index);
	const MaterialData materialData           = material->Get();

	const MaterialBufferData bufferData{
		.ambient           = materialData.ambient,
		.diffuse           = materialData.diffuse,
		.specular          = materialData.specular,
		.diffuseTexUVInfo  = material->GetDiffuseUVInfo(),
		.specularTexUVInfo = material->GetSpecularUVInfo(),
		.diffuseTexIndex   = material->GetDiffuseIndex(),
		.specularTexIndex  = material->GetSpecularIndex(),
		.shininess         = materialData.shininess
	};

	memcpy(bufferOffset + materialOffset, &bufferData, strideSize);
}

void MaterialBuffers::Update(const std::vector<size_t>& indices) const noexcept
{
	std::uint8_t* bufferOffset  = m_buffers.CPUHandle();
	constexpr size_t strideSize = GetStride();

	auto& materials = m_elements.Get();

	for (size_t index : indices)
	{
		const std::shared_ptr<Material>& material = materials.at(index);
		const MaterialData materialData           = material->Get();

		const MaterialBufferData bufferData{
			.ambient           = materialData.ambient,
			.diffuse           = materialData.diffuse,
			.specular          = materialData.specular,
			.diffuseTexUVInfo  = material->GetDiffuseUVInfo(),
			.specularTexUVInfo = material->GetSpecularUVInfo(),
			.diffuseTexIndex   = material->GetDiffuseIndex(),
			.specularTexIndex  = material->GetSpecularIndex(),
			.shininess         = materialData.shininess
		};

		const size_t materialOffset = index * strideSize;
		memcpy(bufferOffset + materialOffset, &bufferData, strideSize);
	}
}

// MeshBounds Buffer
void MeshBoundsBuffers::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
) const noexcept {
	descriptorBuffer.SetStorageBufferDescriptor(m_boundsBuffer, bindingSlot, 0u);
}

void MeshBoundsBuffers::CreateBuffer(size_t boundsCount)
{
	constexpr size_t strideSize      = GetStride();
	const auto meshBoundsBuffersSize = static_cast<VkDeviceSize>(strideSize * boundsCount);

	m_boundsBuffer.Create(meshBoundsBuffersSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
}

void MeshBoundsBuffers::Remove(size_t index) noexcept
{
	m_availableIndices.at(index) = true;
}

size_t MeshBoundsBuffers::Add(const MeshBounds& bounds) noexcept
{
	auto oBoundsIndex = GetFirstAvailableIndex();

	size_t boundsIndex = 0u;

	if (oBoundsIndex)
		boundsIndex = oBoundsIndex.value();
	else
	{
		boundsIndex = GetCount();

		// +1 for the new element.
		const size_t newElementCount = boundsIndex + 1u + GetExtraElementAllocationCount();

		// False for the current new element.
		m_availableIndices.emplace_back(false);
		// True for the extra elements.
		m_availableIndices.resize(newElementCount, true);

		CreateBuffer(newElementCount);
	}

	m_boundsData.emplace(
		BoundsData{
			.positiveBounds = bounds.positiveAxes,
			.negativeBounds = bounds.negativeAxes,
			.index          = static_cast<std::uint32_t>(boundsIndex)
		}
	);

	return boundsIndex;
}

std::vector<size_t> MeshBoundsBuffers::AddMultiple(
	const std::vector<MeshBounds>& multipleBounds
) noexcept {
	std::vector<size_t> freeIndices = GetAvailableIndices();

	for (size_t index = 0u; index < std::size(freeIndices); ++index)
	{
		const size_t freeIndex   = freeIndices.at(index);
		const MeshBounds& bounds = multipleBounds.at(index);

		m_boundsData.emplace(
			BoundsData{
				.positiveBounds = bounds.positiveAxes,
				.negativeBounds = bounds.negativeAxes,
				.index          = static_cast<std::uint32_t>(freeIndex)
			}
		);
	}

	const size_t newElementCount = std::size(multipleBounds);
	const size_t freeIndexCount  = std::size(freeIndices);

	if (newElementCount > freeIndexCount)
	{
		for (size_t index = freeIndexCount; index < newElementCount; ++index)
		{
			const MeshBounds& bounds = multipleBounds.at(index);

			m_boundsData.emplace(
				BoundsData{
					.positiveBounds = bounds.positiveAxes,
					.negativeBounds = bounds.negativeAxes,
					.index          = static_cast<std::uint32_t>(index)
				}
			);

			freeIndices.emplace_back(index);
			m_availableIndices.emplace_back(false);
		}

		const size_t newExtraElementCount = newElementCount + GetExtraElementAllocationCount();

		// True for the extra elements.
		m_availableIndices.resize(newExtraElementCount, true);

		CreateBuffer(newExtraElementCount);
	}

	// Now these are the new used indices.
	freeIndices.resize(newElementCount);

	return freeIndices;
}

void MeshBoundsBuffers::Update() noexcept
{
	std::uint8_t* bufferOffset  = m_boundsBuffer.CPUHandle();
	constexpr size_t strideSize = GetStride();

	while (!std::empty(m_boundsData))
	{
		const BoundsData& boundsData = m_boundsData.front();
		m_boundsData.pop();

		const size_t boundsOffset = boundsData.index * strideSize;
		memcpy(bufferOffset + boundsOffset, &boundsData, strideSize);
	}

	// Release the internal memory of the m_boundsData container, as pop wouldn't.
	m_boundsData = std::queue<BoundsData>{};
}
