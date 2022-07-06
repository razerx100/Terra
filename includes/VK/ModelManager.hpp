#ifndef MODEL_MANAGER_HPP_
#define MODEL_MANAGER_HPP
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

class ModelManagerVertex {
public:
	virtual ~ModelManagerVertex() = default;

	void AddInstance(std::shared_ptr<IModel>&& model) noexcept;
	void AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept;

	virtual void BindInputs(VkCommandBuffer commandBuffer) const noexcept = 0;

	void BindInstances(VkCommandBuffer commandBuffer) const noexcept;

private:
	std::vector<ModelInstance> m_modelInstances;
};

class ModelManagerPerVertex : public ModelManagerVertex {
public:
	ModelManagerPerVertex(
		std::shared_ptr<GpuBuffer> vertexBuffer, std::shared_ptr<GpuBuffer> indexBuffer
	) noexcept;

	void BindInputs(VkCommandBuffer commandBuffer) const noexcept final;

private:
	std::shared_ptr<GpuBuffer> m_vertexBuffer;
	std::shared_ptr<GpuBuffer> m_indexBuffer;
};

class ModelManagerGVertex : public ModelManagerVertex {
public:
	ModelManagerGVertex(std::shared_ptr<GpuBuffer> indexBuffer) noexcept;

	void BindInputs(VkCommandBuffer commandBuffer) const noexcept final;

private:
	std::shared_ptr<GpuBuffer> m_indexBuffer;
};
#endif
