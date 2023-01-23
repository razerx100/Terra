#include <RenderEngine.hpp>

RenderEngine::RenderEngine() noexcept
	: m_backgroundColour{ {0.0001f, 0.0001f, 0.0001f, 0.0001f } } {}

void RenderEngine::AcquireOwnerShipCompute(VkCommandBuffer computeCmdBuffer) noexcept {}
void RenderEngine::CreateBuffers(VkDevice device) noexcept {}
void RenderEngine::CopyData() noexcept {}

void RenderEngine::SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept {
	m_backgroundColour = {
			{colourVector[0], colourVector[1], colourVector[2], colourVector[3]}
	};
}

void RenderEngine::SetShaderPath(const wchar_t* path) noexcept {
	m_shaderPath = path;
}
