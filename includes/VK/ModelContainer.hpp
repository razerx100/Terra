#ifndef MODEL_CONTAINER_HPP_
#define MODEL_CONTAINER_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <IModel.hpp>
#include <BindInstanceGFX.hpp>
#include <string>
#include <memory>

class ModelContainer {
public:
	ModelContainer(std::string shaderPath) noexcept;

	void AddModel(
		VkDevice device, const IModel* const modelRef
	);

	void InitPipelines(VkDevice device);
	void CreateBuffers(VkDevice device);
	void CopyData(std::atomic_size_t& workCount);
	void RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer);
	void ReleaseUploadBuffers();

	void BindCommands(VkCommandBuffer commandBuffer) noexcept;

private:
	using Pipeline =
		std::pair<std::unique_ptr<PipelineObjectGFX>, std::shared_ptr<PipelineLayout>>;

	Pipeline CreatePipeline(
		VkDevice device, const VertexLayout& layout
	) const;

private:
	std::unique_ptr<BindInstanceGFX> m_bindInstance;

	std::string m_shaderPath;
};
#endif
