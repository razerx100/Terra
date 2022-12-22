#ifndef RENDER_ENGINE_INDIRECT_DRAW_HPP_
#define RENDER_ENGINE_INDIRECT_DRAW_HPP_
#include <RenderEngine.hpp>

class RenderEngineIndirectDraw final : public RenderEngine {
public:
	void ExecutePreRenderStage(
		VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
	) override;
	void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) override;
	void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) override;
	void ExecutePostRenderStage() override;
	void ConstructPipelines(std::uint32_t frameCount) override;
};
#endif
