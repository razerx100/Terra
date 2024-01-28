#ifndef GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#define GRAPHICS_PIPELINE_MESH_SHADER_HPP_
#include <vector>
#include <GraphicsPipelineBase.hpp>
#include <VkExtensionManager.hpp>

class GraphicsPipelineMeshShader : public GraphicsPipelineBase {
public:
	void CreateGraphicsPipeline(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath
	) noexcept final;
	void ConfigureGraphicsPipeline(const std::wstring& fragmentShader) noexcept;
	void AddModelDetails(
		std::uint32_t meshletCount, std::uint32_t meshletOffset, std::uint32_t modelIndex
	) noexcept;

	void DrawModels(VkCommandBuffer graphicsCmdBuffer) const noexcept;

private:
	[[nodiscard]]
	std::unique_ptr<VkPipelineObject> CreateGraphicsPipelineMS(
		VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
		const std::wstring& shaderPath, const std::wstring& fragmentShader,
		const std::wstring& meshShader
	) const noexcept;

	void AddPipelineLayout(VkPipelineLayout pipelineLayout) noexcept;

private:
	struct ModelDetails {
		std::uint32_t modelIndex;
		std::uint32_t meshletOffset;
		std::uint32_t meshletCount;
	};

private:
	VkPipelineLayout m_graphicsLayout;
	std::vector<ModelDetails> m_modelDetails;

	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkExtMeshShader
	};

public:
	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }
};
#endif
