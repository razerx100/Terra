#ifndef RENDER_ENGINE_MESH_SHADER_HPP_
#define RENDER_ENGINE_MESH_SHADER_HPP_
#include <memory>
#include <PipelineLayout.hpp>
#include <RenderEngineBase.hpp>

class RenderEngineMeshShader : public RenderEngineBase {
public:
	RenderEngineMeshShader(VkDevice device);

private:
	[[nodiscard]]
	std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
		VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
	) const noexcept final;
};
#endif
