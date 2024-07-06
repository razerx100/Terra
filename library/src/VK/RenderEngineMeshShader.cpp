#include <RenderEngineMeshShader.hpp>

void RenderEngineMSDeviceExtension::SetDeviceExtensions(
	VkDeviceExtensionManager& extensionManager
) noexcept {
	RenderEngineDeviceExtension::SetDeviceExtensions(extensionManager);

	extensionManager.AddExtensions(ModelBundleMS::GetRequiredExtensions());
}

RenderEngineMS::RenderEngineMS(
	const VkDeviceManager& deviceManager, std::shared_ptr<ThreadPool> threadPool, size_t frameCount
) : RenderEngineCommon{ deviceManager, std::move(threadPool), frameCount }
{
	// The layout shouldn't change throughout the runtime.
	m_modelManager.SetDescriptorBufferLayout(m_graphicsDescriptorBuffers);

	for (auto& descriptorBuffer : m_graphicsDescriptorBuffers)
		descriptorBuffer.CreateBuffer();
}

std::uint32_t RenderEngineMS::AddModel(
	std::shared_ptr<ModelMS>&& model, const std::wstring& fragmentShader
) {
	return 0u;
}

std::uint32_t RenderEngineMS::AddModelBundle(
	std::vector<std::shared_ptr<ModelMS>>&& modelBundle, const std::wstring& fragmentShader
) {
	return 0u;
}

std::uint32_t RenderEngineMS::AddMeshBundle(std::unique_ptr<MeshBundleMS> meshBundle)
{
	return 0u;
}

void RenderEngineMS::Render(
	size_t frameIndex, const VKFramebuffer& frameBuffer, VkExtent2D renderArea
) {

}

ModelManagerMS RenderEngineMS::GetModelManager(
	const VkDeviceManager& deviceManager, MemoryManager* memoryManager,
	StagingBufferManager* stagingBufferMan, std::uint32_t frameCount
) {
	return ModelManagerMS{
		deviceManager.GetLogicalDevice(), memoryManager, stagingBufferMan, frameCount
	};
}
