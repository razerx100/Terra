#ifndef RENDER_ENGINE_HPP_
#define RENDER_ENGINE_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include <memory>

#include <IModel.hpp>

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

	virtual void RecordModelDataSet(
		const std::vector<std::shared_ptr<IModel>>& models, const std::wstring& fragmentShader
	) noexcept = 0;
	virtual void CreateBuffers(VkDevice device) noexcept = 0;
	virtual void BindResourcesToMemory(VkDevice device) = 0;
	virtual void CopyData() noexcept = 0;
	virtual void RecordCopy(VkCommandBuffer copyBuffer) noexcept = 0;
	virtual void ReleaseUploadResources() noexcept = 0;
	virtual void AcquireOwnerShip(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept = 0;
	virtual void ReleaseOwnership(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept = 0;

	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept;
	void SetShaderPath(const wchar_t* path) noexcept;

protected:
	VkClearColorValue m_backgroundColour;
	std::wstring m_shaderPath;
};
#endif
