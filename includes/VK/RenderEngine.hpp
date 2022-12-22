#ifndef RENDER_ENGINE_HPP_
#define RENDER_ENGINE_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <array>
#include <cstdint>

class RenderEngine {
public:
	RenderEngine() noexcept;
	virtual ~RenderEngine() = default;

	virtual void ExecutePreRenderStage(
		VkCommandBuffer graphicsCmdBuffer, size_t frameIndex
	) = 0;
	virtual void RecordDrawCommands(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) = 0;
	virtual void Present(VkCommandBuffer graphicsCmdBuffer, size_t frameIndex) = 0;
	virtual void ExecutePostRenderStage() = 0;
	virtual void ConstructPipelines(std::uint32_t frameCount) = 0;

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept;
	void SetShaderPath(const wchar_t* path) noexcept;

protected:
	VkClearColorValue m_backgroundColour;
	std::wstring m_shaderPath;
};
#endif
