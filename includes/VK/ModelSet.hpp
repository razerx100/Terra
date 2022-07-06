#ifndef MODEL_SET_HPP_
#define MODEL_SET_HPP
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <PipelineLayout.hpp>
#include <VkBuffers.hpp>

#include <IModel.hpp>

class ModelInstance {
public:
	ModelInstance(std::shared_ptr<IModel>&& model) noexcept;

	void AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept;
	void Draw(VkCommandBuffer commandBuffer) const noexcept;

private:
	std::shared_ptr<IModel> m_model;
	VkPipelineLayout m_pPipelineLayout;
};

class ModelSetVertex {
public:
	virtual ~ModelSetVertex() = default;

	void AddInstance(std::shared_ptr<IModel>&& model) noexcept;
	void AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept;
	void DrawInstances(VkCommandBuffer commandBuffer) const noexcept;

	virtual void BindInputs(VkCommandBuffer commandBuffer) const noexcept = 0;

private:
	std::vector<ModelInstance> m_modelInstances;
};

class ModelSetPerVertex final : public ModelSetVertex {
public:
	ModelSetPerVertex(
		std::shared_ptr<GpuBuffer> vertexBuffer, std::shared_ptr<GpuBuffer> indexBuffer
	) noexcept;

	void BindInputs(VkCommandBuffer commandBuffer) const noexcept override;

private:
	std::shared_ptr<GpuBuffer> m_vertexBuffer;
	std::shared_ptr<GpuBuffer> m_indexBuffer;
};

class ModelSetGVertex final : public ModelSetVertex {
public:
	ModelSetGVertex(std::shared_ptr<GpuBuffer> indexBuffer) noexcept;

	void BindInputs(VkCommandBuffer commandBuffer) const noexcept override;

private:
	std::shared_ptr<GpuBuffer> m_indexBuffer;
};
#endif
