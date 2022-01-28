#ifndef __BIND_INSTANCE_GFX_HPP__
#define __BIND_INSTANCE_GFX_HPP__
#include <IBindInstanceGFX.hpp>
#include <vector>
#include <cstdint>

class BindInstanceGFX : public IBindInstanceGFX {
public:
	BindInstanceGFX(bool textureAvailable) noexcept;
	BindInstanceGFX(
		bool textureAvailable,
		std::unique_ptr<IPipelineObject> pso,
		std::unique_ptr<IPipelineLayout> layout
	) noexcept;

	void AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept override;
	void AddPipelineLayout(std::unique_ptr<IPipelineLayout> layout) noexcept override;
	void AddModel(
		VkDevice device, const IModel* const modelRef
	) noexcept override;

	void BindCommands(VkCommandBuffer commandBuffer) noexcept override;

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

		void Draw(VkCommandBuffer commandBuffer) noexcept;

	private:
		VkDevice m_deviceRef;
		const IModel* const m_modelRef;
		VkBuffer m_vertexBuffer;
		VkBuffer m_indexBuffer;
		VkDeviceSize m_vertexOffset;
		std::uint32_t m_indexCount;
	};

private:
	bool m_textureAvailable;
	std::unique_ptr<IPipelineLayout> m_pipelineLayout;
	std::unique_ptr<IPipelineObject> m_pso;
	std::vector<std::unique_ptr<ModelRaw>> m_modelsRaw;
};
#endif