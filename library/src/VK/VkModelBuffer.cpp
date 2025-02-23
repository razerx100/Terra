#include <VkModelBuffer.hpp>

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
			const std::shared_ptr<Model>& model = m_elements.at(index);

			// Vertex Data
			{
				using namespace DirectX;

				const XMMATRIX modelMat = model->GetModelMatrix();

				const ModelVertexData modelVertexData
				{
					.modelMatrix   = modelMat,
					// The normal matrix is the transpose of the inversed model matrix. Not doing this
					// to flip to Column major.
					.normalMatrix  = XMMatrixTranspose(XMMatrixInverse(nullptr, modelMat)),
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

void ModelBuffers::Remove(const std::vector<std::uint32_t>& indices) noexcept
{
	for (std::uint32_t index : indices)
		Remove(index);
}

void ModelBuffers::Remove(const std::vector<size_t>& indices) noexcept
{
	for (size_t index : indices)
		Remove(index);
}
