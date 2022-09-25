#ifndef MODEL_MANAGER_HPP_
#define MODEL_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <string>
#include <memory>
#include <vector>
#include <PerFrameBuffers.hpp>

#include <IModel.hpp>
#include <RenderPipeline.hpp>

class ModelManager {
public:
	ModelManager(
		VkDevice device, std::vector<std::uint32_t> queueFamilyIndices,
		std::uint32_t bufferCount
	) noexcept;

	void SetShaderPath(std::wstring path) noexcept;
	void AddModels(std::vector<std::shared_ptr<IModel>>&& models);

	void InitPipelines(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	);
	void BindMemories(VkDevice device);
	void RecordUploadBuffers(VkCommandBuffer copyBuffer) noexcept;
	void ReleaseUploadBuffers();
	void AddModelInputs(
		VkDevice device,
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);
	void CreateBuffers(VkDevice device, std::uint32_t bufferCount) noexcept;
	void CopyData() noexcept;

	void BindCommands(VkCommandBuffer commandBuffer, size_t frameIndex) const noexcept;

private:
	using Pipeline =
		std::pair<std::unique_ptr<PipelineObjectGFX>, std::unique_ptr<PipelineLayout>>;

	Pipeline CreatePipeline(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const;

private:
	RenderPipeline m_renderPipeline;
	PerFrameBuffers m_perFrameBuffers;

	std::wstring m_shaderPath;
};
#endif
