#include <MeshManager.hpp>

// Mesh Manager VS Individual
MeshManagerVSIndividual::MeshManagerVSIndividual(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3
) : MeshManager{},
	m_vertexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}, m_indexBuffer{
		device, memoryManager, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		queueIndices3.ResolveQueueIndices<QueueIndicesTG>()
	}
{}

void MeshManagerVSIndividual::CopyOldBuffers(const VKCommandBuffer& transferCmdBuffer) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_indexBuffer.CopyOldBuffer(transferCmdBuffer);
		m_vertexBuffer.CopyOldBuffer(transferCmdBuffer);

		m_oldBufferCopyNecessary = false;
	}
}

void MeshManagerVSIndividual::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	VkMeshBundleVS& vkMeshBundle, TemporaryDataBufferGPU& tempBuffer
) {
	vkMeshBundle.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, tempBuffer
	);
}

void MeshManagerVSIndividual::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	VkMeshBundleVS& vkMeshBundle = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData = vkMeshBundle.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData = vkMeshBundle.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);
	}
}

// Mesh Manager VS Indirect
MeshManagerVSIndirect::MeshManagerVSIndirect(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3
) : MeshManager{},
	m_vertexBuffer{
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
	}
{}

void MeshManagerVSIndirect::CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept
{
	if (m_oldBufferCopyNecessary)
	{
		m_vertexBuffer.CopyOldBuffer(transferBuffer);
		m_indexBuffer.CopyOldBuffer(transferBuffer);
		m_perMeshDataBuffer.CopyOldBuffer(transferBuffer);
		m_perMeshBundleDataBuffer.CopyOldBuffer(transferBuffer);

		m_oldBufferCopyNecessary = false;
	}
}

void MeshManagerVSIndirect::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	VkMeshBundleVS& vkMeshBundle, TemporaryDataBufferGPU& tempBuffer
) {
	vkMeshBundle.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_indexBuffer, m_perMeshDataBuffer,
		m_perMeshBundleDataBuffer, tempBuffer
	);
}

void MeshManagerVSIndirect::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	VkMeshBundleVS& vkMeshBundle = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData        = vkMeshBundle.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& indexSharedData         = vkMeshBundle.GetIndexSharedData();
		m_indexBuffer.RelinquishMemory(indexSharedData);

		const SharedBufferData& perMeshSharedData       = vkMeshBundle.GetPerMeshSharedData();
		m_perMeshDataBuffer.RelinquishMemory(perMeshSharedData);

		const SharedBufferData& perMeshBundleSharedData = vkMeshBundle.GetPerMeshBundleSharedData();
		m_perMeshBundleDataBuffer.RelinquishMemory(perMeshBundleSharedData);
	}
}

void MeshManagerVSIndirect::SetDescriptorBufferLayoutCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const noexcept {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
	{
		descriptorBuffer.AddBinding(
			s_perMeshDataBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
		descriptorBuffer.AddBinding(
			s_perMeshBundleDataBindingSlot, csSetLayoutIndex, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u,
			VK_SHADER_STAGE_COMPUTE_BIT
		);
	}
}

void MeshManagerVSIndirect::SetDescriptorBufferCS(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t csSetLayoutIndex
) const {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
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

// Mesh Manager MS
MeshManagerMS::MeshManagerMS(
	VkDevice device, MemoryManager* memoryManager, QueueIndices3 queueIndices3
) : MeshManager{},
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

void MeshManagerMS::ConfigureMeshBundle(
	std::unique_ptr<MeshBundleTemporary> meshBundle, StagingBufferManager& stagingBufferMan,
	VkMeshBundleMS& vkMeshBundle, TemporaryDataBufferGPU& tempBuffer
) {
	vkMeshBundle.SetMeshBundle(
		std::move(meshBundle), stagingBufferMan, m_vertexBuffer, m_vertexIndicesBuffer,
		m_primIndicesBuffer, m_perMeshletDataBuffer, tempBuffer
	);
}

void MeshManagerMS::ConfigureRemoveMesh(size_t bundleIndex) noexcept
{
	VkMeshBundleMS& vkMeshBundle = m_meshBundles.at(bundleIndex);

	{
		const SharedBufferData& vertexSharedData        = vkMeshBundle.GetVertexSharedData();
		m_vertexBuffer.RelinquishMemory(vertexSharedData);

		const SharedBufferData& vertexIndicesSharedData = vkMeshBundle.GetVertexIndicesSharedData();
		m_vertexIndicesBuffer.RelinquishMemory(vertexIndicesSharedData);

		const SharedBufferData& primIndicesSharedData   = vkMeshBundle.GetPrimIndicesSharedData();
		m_primIndicesBuffer.RelinquishMemory(primIndicesSharedData);

		const SharedBufferData& perMeshletSharedData    = vkMeshBundle.GetPerMeshletSharedData();
		m_perMeshletDataBuffer.RelinquishMemory(perMeshletSharedData);
	}
}

void MeshManagerMS::SetDescriptorBufferLayout(
	std::vector<VkDescriptorBuffer>& descriptorBuffers, size_t msSetLayoutIndex
) const noexcept {
	for (VkDescriptorBuffer& descriptorBuffer : descriptorBuffers)
	{
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

void MeshManagerMS::SetDescriptorBuffer(
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

void MeshManagerMS::CopyOldBuffers(const VKCommandBuffer& transferBuffer) noexcept
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
