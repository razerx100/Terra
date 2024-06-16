#include <RenderEngine.hpp>

RenderEngine::RenderEngine(
	VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
	VkQueueFamilyMananger const* queueFamilyManager, std::shared_ptr<ThreadPool> threadPool,
	size_t frameCount
) : m_threadPool{ std::move(threadPool) },
	m_memoryManager{ physicalDevice, logicalDevice, 20_MB, 400_KB },
	m_graphicsQueue{
		logicalDevice,
		queueFamilyManager->GetQueue(QueueType::GraphicsQueue),
		queueFamilyManager->GetIndex(QueueType::GraphicsQueue)
	}, m_transferQueue{
		logicalDevice,
		queueFamilyManager->GetQueue(QueueType::TransferQueue),
		queueFamilyManager->GetIndex(QueueType::TransferQueue)
	},
	m_stagingManager{
		logicalDevice, &m_memoryManager, &m_transferQueue, m_threadPool.get(), queueFamilyManager
	}, m_graphicsDescriptorBuffers{},
	m_textureStorage{ logicalDevice, &m_memoryManager },
	m_textureManager{ logicalDevice, &m_memoryManager },
	m_materialBuffers{ logicalDevice, &m_memoryManager },
	m_cameraManager{ logicalDevice, &m_memoryManager },
	m_depthBuffers{ logicalDevice, &m_memoryManager }, m_renderPass{ logicalDevice },
	m_backgroundColour{ {0.0001f, 0.0001f, 0.0001f, 0.0001f } }
{
	for (size_t _ = 0u; _ < frameCount; ++_)
		m_graphicsDescriptorBuffers.emplace_back(logicalDevice, &m_memoryManager);

	m_graphicsQueue.CreateCommandBuffers(static_cast<std::uint32_t>(frameCount));
	m_transferQueue.CreateCommandBuffers(1u);
}

size_t RenderEngine::AddMaterial(std::shared_ptr<Material> material)
{
	const size_t index = m_materialBuffers.Add(std::move(material));

	m_materialBuffers.Update(index);

	return index;
}

std::vector<size_t> RenderEngine::AddMaterials(std::vector<std::shared_ptr<Material>>&& materials)
{
	std::vector<size_t> indices = m_materialBuffers.AddMultiple(std::move(materials));

	m_materialBuffers.Update(indices);

	return indices;
}

void RenderEngine::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept
{
	m_backgroundColour = {
			{ colourVector.at(0u), colourVector.at(1), colourVector.at(2), colourVector.at(3) }
	};
}
