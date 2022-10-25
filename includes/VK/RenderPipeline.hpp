#ifndef RENDER_PIPELINE_HPP_
#define RENDER_PIPELINE_HPP_
#include <vector>
#include <memory>
#include <VKPipelineObject.hpp>
#include <PipelineLayout.hpp>
#include <VkResourceViews.hpp>

#include <IModel.hpp>

class RenderPipeline {
public:
	RenderPipeline(
		VkDevice device, std::vector<std::uint32_t> queueFamilyIndices,
		std::uint32_t bufferCount
	) noexcept;

	void AddGraphicsPipelineObject(std::unique_ptr<VkPipelineObject> graphicsPSO) noexcept;
	void AddGraphicsPipelineLayout(std::unique_ptr<PipelineLayout> graphicsLayout) noexcept;
	void RecordIndirectArguments(const std::vector<std::shared_ptr<IModel>>& models) noexcept;

	void CreateBuffers(VkDevice device) noexcept;
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
	VkUploadableBufferResourceView m_commandBuffers;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	std::uint32_t m_bufferCount;
	std::uint32_t m_modelCount;
	std::vector<VkDrawIndexedIndirectCommand> m_indirectCommands;
};
#endif
