#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_
#include <vector>
#include <memory>
#include <VKPipelineObject.hpp>
#include <PipelineLayout.hpp>
#include <VkResourceViews.hpp>

#include <IModel.hpp>

struct ModelConstantBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	float padding[3];
};

class RenderPipeline {
public:
	RenderPipeline(VkDevice device, std::vector<std::uint32_t> queueFamilyIndices) noexcept;

	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void AddGraphicsPipelineObject(std::unique_ptr<VkPipelineObject> graphicsPSO) noexcept;
	void AddGraphicsPipelineLayout(std::unique_ptr<PipelineLayout> graphicsLayout) noexcept;

	void CreateBuffers(VkDevice device, std::uint32_t bufferCount) noexcept;
	void UpdateModelData(VkDeviceSize frameIndex) const noexcept;
	void BindResourceToMemory(VkDevice device);
	void BindGraphicsPipeline(
		VkCommandBuffer graphicsCmdBuffer, VkDescriptorSet descriptorSet
	) const noexcept;
	void CopyData() noexcept;
	void RecordCopy(VkCommandBuffer copyBuffer) noexcept;
	void ReleaseUploadResources() noexcept;

	void DrawModels(VkCommandBuffer graphicsCmdBuffer, VkDeviceSize frameIndex) const noexcept;

private:
	std::unique_ptr<PipelineLayout> m_graphicsPipelineLayout;
	std::unique_ptr<VkPipelineObject> m_graphicsPSO;
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
	VkResourceView m_modelBuffers;
	VkUploadableBufferResourceView m_commandBuffers;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	std::uint32_t m_bufferCount;
};
#endif
