#ifndef __MODEL_CONTAINER_HPP__
#define __MODEL_CONTAINER_HPP__
#include <IModelContainer.hpp>
#include <IBindInstanceGFX.hpp>
#include <vector>
#include <string>
#include <memory>

class ModelContainer : public IModelContainer {
public:
	ModelContainer(const char* shaderPath) noexcept;

	void AddModel(
		VkDevice device, const IModel* const modelRef,
		bool texture
	) override;

	void CreateBuffers(VkDevice device) override;
	void CopyData() override;
	void RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer) override;
	void ReleaseUploadBuffers(VkDevice device) override;

	void BindCommands(VkCommandBuffer commandBuffer) noexcept override;

private:
	struct InstanceData {
		bool available;
		size_t index;
	};

private:
	void InitNewInstance(InstanceData& instanceData, bool texure) noexcept;
	void AddColoredModel(VkDevice device, const IModel* const modelRef);
	void AddTexturedModel(VkDevice device, const IModel* const modelRef);

private:
	std::vector<std::unique_ptr<IBindInstanceGFX>> m_bindInstances;
	InstanceData m_coloredInstanceData;
	InstanceData m_texturedInstanceData;

	std::string m_shaderPath;
};
#endif
