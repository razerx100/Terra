#include <VkModelBuffer.hpp>

namespace Terra
{
// Model Buffers
void ModelBuffers::ExtendModelBuffers()
{
	const size_t currentModelCount = m_modelContainer->GetModelCount();

	CreateBuffer(currentModelCount);
}

void ModelBuffers::CreateBuffer(size_t modelCount)
{
	// Vertex Data
	{
		constexpr size_t strideSize = GetVertexStride();

		m_modelBuffersInstanceSize = static_cast<VkDeviceSize>(strideSize * modelCount);

		const VkDeviceSize modelBufferTotalSize
			= m_modelBuffersInstanceSize * m_bufferInstanceCount;

		// The vertex buffer can be accessed by different types of command queues.
		m_vertexModelBuffers.Create(
			modelBufferTotalSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, m_modelBuffersQueueIndices
		);
	}

	// Fragment Data
	{
		constexpr size_t strideSize = GetFragmentStride();

		m_modelBuffersFragmentInstanceSize = static_cast<VkDeviceSize>(strideSize * modelCount);

		const VkDeviceSize modelBufferTotalSize
			= m_modelBuffersFragmentInstanceSize * m_bufferInstanceCount;

		// The fragment buffer should only be accessed by the graphics command queue.
		m_fragmentModelBuffers.Create(modelBufferTotalSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});
	}
}

void ModelBuffers::SetDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot,
	size_t setLayoutIndex
) const {
	const auto bufferOffset = static_cast<VkDeviceAddress>(
		frameIndex * m_modelBuffersInstanceSize
	);

	descriptorBuffer.SetStorageBufferDescriptor(
		m_vertexModelBuffers, bindingSlot, setLayoutIndex, 0u, bufferOffset,
		m_modelBuffersInstanceSize
	);
}

void ModelBuffers::SetFragmentDescriptorBuffer(
	VkDescriptorBuffer& descriptorBuffer, VkDeviceSize frameIndex, std::uint32_t bindingSlot,
	size_t setLayoutIndex
) const {
	const auto bufferOffset
		= static_cast<VkDeviceAddress>(frameIndex * m_modelBuffersFragmentInstanceSize);

	descriptorBuffer.SetStorageBufferDescriptor(
		m_fragmentModelBuffers, bindingSlot, setLayoutIndex, 0u, bufferOffset,
		m_modelBuffersFragmentInstanceSize
	);
}

void ModelBuffers::Update(VkDeviceSize bufferIndex) const noexcept
{
	// Vertex Data
	std::uint8_t* vertexBufferOffset
		= m_vertexModelBuffers.CPUHandle() + bufferIndex * m_modelBuffersInstanceSize;
	constexpr size_t vertexStrideSize = GetVertexStride();
	size_t vertexModelOffset          = 0u;

	// Fragment Data
	std::uint8_t* fragmentBufferOffset
		= m_fragmentModelBuffers.CPUHandle() + bufferIndex * m_modelBuffersFragmentInstanceSize;
	constexpr size_t fragmentStrideSize = GetFragmentStride();
	size_t fragmentModelOffset          = 0u;

	const Callisto::ReusableVector<Model>& models = m_modelContainer->GetModels();

	// All of the models will be here. Even after multiple models have been removed, there
	// should be invalid models there. It is necessary to keep them to preserve the model indices,
	// which is used to keep track of the models both on the CPU and the GPU side.
	for (const Model& model : models)
	{
		// Vertex Data
		{
			using namespace DirectX;

			const XMMATRIX& modelMat = model.GetModelMatrix();

			const ModelVertexData modelVertexData
			{
				.modelMatrix   = modelMat,
				// The normal matrix is the transpose of the inversed model matrix. Not doing
				// this to flip to Column major.
				.normalMatrix  = XMMatrixTranspose(XMMatrixInverse(nullptr, modelMat)),
				.modelOffset   = model.GetModelOffset(),
				.materialIndex = model.GetMaterialIndex(),
				.meshIndex     = model.GetMeshIndex(),
				.modelScale    = model.GetModelScale()
			};

			memcpy(vertexBufferOffset + vertexModelOffset, &modelVertexData, vertexStrideSize);
		}

		// Fragment Data
		{
			const ModelFragmentData modelFragmentData
			{
				.diffuseTexUVInfo  = model.GetDiffuseUVInfo(),
				.specularTexUVInfo = model.GetSpecularUVInfo(),
				.diffuseTexIndex   = model.GetDiffuseIndex(),
				.specularTexIndex  = model.GetSpecularIndex()
			};

			memcpy(
				fragmentBufferOffset + fragmentModelOffset, &modelFragmentData,
				fragmentStrideSize
			);
		}

		// The offsets need to be always increased to keep them consistent.
		vertexModelOffset   += vertexStrideSize;
		fragmentModelOffset += fragmentStrideSize;
	}
}
}
