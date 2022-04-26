#ifndef MODEL_CONTAINER_HPP_
#define MODEL_CONTAINER_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <IModel.hpp>
#include <BindInstanceGFX.hpp>
#include <string>
#include <memory>
#include <vector>

class ModelContainer {
public:
	ModelContainer(std::string shaderPath) noexcept;

	void AddModel(
		VkDevice device, const IModel* const modelRef
	);

	void InitPipelines(
		VkDevice device,
		const std::vector<VkDescriptorSetLayout>& setLayout
	);
	void CreateBuffers(VkDevice device);
	void CopyData(std::atomic_size_t& workCount);
	void RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer);
	void ReleaseUploadBuffers();

	void BindCommands(VkCommandBuffer commandBuffer) const noexcept;

private:
	using Pipeline =
		std::pair<std::unique_ptr<PipelineObjectGFX>, std::shared_ptr<PipelineLayout>>;

	Pipeline CreatePipeline(
		VkDevice device, const VertexLayout& layout,
		const std::vector<VkDescriptorSetLayout>& setLayout
	) const;

private:
	std::unique_ptr<BindInstanceGFX> m_bindInstance;

	std::string m_shaderPath;
};
#endif
