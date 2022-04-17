#ifndef BIND_INSTANCE_GFX_HPP_
#define BIND_INSTANCE_GFX_HPP_
#include <vulkan/vulkan.hpp>
#include <IModel.hpp>
#include <PipelineObjectGFX.hpp>
#include <VertexLayout.hpp>
#include <PipelineLayout.hpp>
#include <vector>
#include <cstdint>

class BindInstanceGFX {
public:
	BindInstanceGFX() noexcept;
	BindInstanceGFX(
		std::unique_ptr<PipelineObjectGFX> pso,
		std::shared_ptr<PipelineLayout> layout
	) noexcept;

	[[nodiscard]]
	VertexLayout GetVertexLayout() const noexcept;

	void AddPSO(std::unique_ptr<PipelineObjectGFX> pso) noexcept;
	void AddPipelineLayout(std::shared_ptr<PipelineLayout> layout) noexcept;
	void AddModel(
		VkDevice device, const IModel* const modelRef
	) noexcept;

	void BindCommands(VkCommandBuffer commandBuffer) noexcept;

private:
	class ModelRaw {
	public:
		ModelRaw(VkDevice device, const IModel* const modelRef) noexcept;
		ModelRaw(
			VkDevice device,
			const IModel* const modelRef,
			VkBuffer vertexBuffer,
			VkBuffer indexBuffer,
			size_t indexCount
		) noexcept;
		~ModelRaw() noexcept;

		void AddVertexBuffer(VkBuffer buffer) noexcept;
		void AddIndexBuffer(VkBuffer buffer, size_t indexCount) noexcept;
		void AddPipelineLayout(std::shared_ptr<PipelineLayout> pipelineLayout) noexcept;

		void Draw(VkCommandBuffer commandBuffer) const noexcept;

	private:
		VkDevice m_deviceRef;
		const IModel* const m_modelRef;
		VkBuffer m_vertexBuffer;
		VkBuffer m_indexBuffer;
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