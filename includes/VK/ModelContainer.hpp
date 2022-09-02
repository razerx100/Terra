#ifndef MODEL_CONTAINER_HPP_
#define MODEL_CONTAINER_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <string>
#include <memory>
#include <vector>
#include <PerFrameBuffers.hpp>

#include <IModel.hpp>
#include <RenderPipeline.hpp>

class ModelContainer {
public:
	ModelContainer(std::string shaderPath, VkDevice device) noexcept;

	void AddModels(std::vector<std::shared_ptr<IModel>>&& models);

	void InitPipelines(VkDevice device, VkDescriptorSetLayout setLayout);
	void BindMemories(VkDevice device);
	void CopyData(std::atomic_size_t& workCount);
	void RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer);
	void ReleaseUploadBuffers();
	void AddModelInputs(
		VkDevice device,
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);

	void BindCommands(VkCommandBuffer commandBuffer) const noexcept;

private:
	using Pipeline =
		std::pair<std::unique_ptr<PipelineObjectGFX>, std::unique_ptr<PipelineLayout>>;

	Pipeline CreatePipeline(VkDevice device, VkDescriptorSetLayout setLayout) const;

private:
	RenderPipeline m_renderPipeline;
	PerFrameBuffers m_pPerFrameBuffers;

	std::string m_shaderPath;
};
#endif
