#ifndef BIND_INSTANCE_GFX_HPP_
#define BIND_INSTANCE_GFX_HPP_
#include <vulkan/vulkan.hpp>
#include <PipelineObjectGFX.hpp>
#include <PipelineLayout.hpp>
#include <VkBuffers.hpp>
#include <vector>
#include <cstdint>
#include <UploadBuffers.hpp>
#include <ModelManager.hpp>

#include <IModel.hpp>

class BindInstanceGFX {
public:
	BindInstanceGFX() = default;
	BindInstanceGFX(
		std::unique_ptr<PipelineObjectGFX> pso, std::unique_ptr<PipelineLayout> layout
	) noexcept;

	virtual ~BindInstanceGFX() = default;

	void AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddPipelineLayout(std::unique_ptr<PipelineLayout> layout) noexcept;

	void BindPipeline(
		VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
	) const noexcept;

	virtual void AddModels(
		VkDevice device, std::vector<std::shared_ptr<IModel>>&& models,
		std::unique_ptr<IModelInputs> modelInputs
	) noexcept = 0;
	virtual void DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept = 0;

private:
	std::unique_ptr<PipelineLayout> m_pipelineLayout;
	std::unique_ptr<PipelineObjectGFX> m_pso;

protected:
	std::vector<std::unique_ptr<ModelManagerVertex>> m_models;
};

class BindInstancePerVertex : public BindInstanceGFX {
public:
	void AddModels(
		VkDevice device, std::vector<std::shared_ptr<IModel>>&& models,
		std::unique_ptr<IModelInputs> modelInputs
	) noexcept final;
	void DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept final;
};

class BindInstanceGVertex : public BindInstanceGFX {
public:
	void AddModels(
		VkDevice device, std::vector<std::shared_ptr<IModel>>&& models,
		std::unique_ptr<IModelInputs> modelInputs
	) noexcept final;
	void DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept final;

private:
	std::shared_ptr<GpuBuffer> m_vertexBuffer;
};
#endif