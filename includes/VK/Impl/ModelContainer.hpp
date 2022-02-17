#ifndef __MODEL_CONTAINER_HPP__
#define __MODEL_CONTAINER_HPP__
#include <IModelContainer.hpp>
#include <IBindInstanceGFX.hpp>
#include <string>
#include <memory>

class ModelContainer : public IModelContainer {
public:
	ModelContainer(const char* shaderPath) noexcept;

	void AddModel(
		VkDevice device, const IModel* const modelRef
	) override;

	void InitPipelines(VkDevice device) override;
	void CreateBuffers(VkDevice device) override;
	void CopyData() override;
	void RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer) override;
	void ReleaseUploadBuffers(VkDevice device) override;

	void BindCommands(VkCommandBuffer commandBuffer) noexcept override;

private:
	using Pipeline =
		std::pair<std::unique_ptr<IPipelineObject>, std::unique_ptr<IPipelineLayout>>;

	Pipeline CreatePipeline(
		VkDevice device, const VertexLayout& layout
	) const;

private:
	std::unique_ptr<IBindInstanceGFX> m_bindInstance;

	std::string m_shaderPath;
};
#endif
