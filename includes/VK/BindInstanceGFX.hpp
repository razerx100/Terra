#ifndef BIND_INSTANCE_GFX_HPP_
#define BIND_INSTANCE_GFX_HPP_
#include <vulkan/vulkan.hpp>
#include <PipelineObjectGFX.hpp>
#include <PipelineLayout.hpp>
#include <VkBuffers.hpp>
#include <vector>
#include <cstdint>
#include <UploadBuffers.hpp>

#include <VertexLayout.hpp>
#include <IModel.hpp>

class BindInstanceGFX {
public:
	BindInstanceGFX() noexcept;
	BindInstanceGFX(
		std::unique_ptr<PipelineObjectGFX> pso, std::shared_ptr<PipelineLayout> layout
	) noexcept;

	[[nodiscard]]
	VertexLayout GetVertexLayout() const noexcept;

	void AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddPipelineLayout(std::shared_ptr<PipelineLayout> layout) noexcept;
	void AddModel(VkDevice device, std::shared_ptr<IModel>&& model) noexcept;

	void DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept;
	void BindPipeline(
		VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
	) const noexcept;

private:
	class ModelRaw {
	public:
		ModelRaw(VkDevice device, std::shared_ptr<IModel>&& model) noexcept;
		ModelRaw(
			VkDevice device,
			std::shared_ptr<GpuBuffer> vertexBuffer,
			std::shared_ptr<GpuBuffer> indexBuffer,
			size_t indexCount,
			std::shared_ptr<IModel>&& model
		) noexcept;

		void AddVertexBuffer(std::shared_ptr<GpuBuffer> buffer) noexcept;
		void AddIndexBuffer(std::shared_ptr<GpuBuffer> buffer, size_t indexCount) noexcept;
		void AddPipelineLayout(std::shared_ptr<PipelineLayout> pipelineLayout) noexcept;

		void Draw(VkCommandBuffer commandBuffer) const noexcept;

	private:
		VkDevice m_deviceRef;
		std::shared_ptr<IModel> m_model;
		std::shared_ptr<GpuBuffer> m_vertexBuffer;
		std::shared_ptr<GpuBuffer> m_indexBuffer;
		VkDeviceSize m_vertexOffset;
		std::uint32_t m_indexCount;
		std::shared_ptr<PipelineLayout> m_pPipelineLayout;
	};

private:
	std::shared_ptr<PipelineLayout> m_pipelineLayout;
	std::unique_ptr<PipelineObjectGFX> m_pso;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;

	bool m_vertexLayoutAvailable;
	VertexLayout m_vertexLayout;
};
#endif