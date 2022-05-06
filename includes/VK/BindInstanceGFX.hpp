#ifndef BIND_INSTANCE_GFX_HPP_
#define BIND_INSTANCE_GFX_HPP_
#include <vulkan/vulkan.hpp>
#include <IModel.hpp>
#include <PipelineObjectGFX.hpp>
#include <VertexLayout.hpp>
#include <PipelineLayout.hpp>
#include <VkBuffers.hpp>
#include <vector>
#include <cstdint>
#include <UploadBuffers.hpp>

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
	void InitializeTransformBuffer(VkDevice logicalDevice, VkPhysicalDevice physicalDevice);

	void BindCommands(
		VkDevice device,
		VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
	) const noexcept;

private:
	class ModelRaw {
	public:
		ModelRaw(VkDevice device, const IModel* const modelRef) noexcept;
		ModelRaw(
			VkDevice device,
			const IModel* const modelRef,
			std::shared_ptr<GpuBuffer> vertexBuffer,
			std::shared_ptr<GpuBuffer> indexBuffer,
			size_t indexCount
		) noexcept;

		void AddVertexBuffer(std::shared_ptr<GpuBuffer> buffer) noexcept;
		void AddIndexBuffer(std::shared_ptr<GpuBuffer> buffer, size_t indexCount) noexcept;
		void AddPipelineLayout(std::shared_ptr<PipelineLayout> pipelineLayout) noexcept;

		void UpdateBuffers(
			VkDevice device,
			UploadBufferSingle* pBuffer
		) const noexcept;
		void Draw(VkCommandBuffer commandBuffer) const noexcept;

	private:
		VkDevice m_deviceRef;
		const IModel* const m_modelRef;
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
	std::unique_ptr<UploadBufferSingle> m_pTransformBuffer;
};
#endif