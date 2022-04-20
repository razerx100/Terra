#ifndef RENDERER_VK_HPP_
#define RENDERER_VK_HPP_
#include <Renderer.hpp>
#include <vulkan/vulkan.hpp>
#include <string>

class RendererVK final : public Renderer {
public:
	RendererVK(
		const char* appName,
		void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height,
		std::uint32_t bufferCount
	);
	~RendererVK() noexcept override;

	void Resize(std::uint32_t width, std::uint32_t height) override;
	void GetMonitorCoordinates(
		std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
	) override;

	[[nodiscard]]
	size_t RegisterResource(
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
	) override;

	void SetThreadPool(std::shared_ptr<IThreadPool> threadPoolArg) noexcept override;
	void SetBackgroundColour(const Ceres::Float32_4& colourVector) noexcept override;
	void SubmitModel(const IModel* const modelRef) override;
	void Render() override;
	void WaitForAsyncTasks() override;

	void SetShaderPath(const char* path) noexcept override;
	void InitResourceBasedObjects() override;
	void ProcessData() override;

private:
	VkClearValue m_backgroundColour;
	const std::string m_appName;

	std::string m_shaderPath;
	std::uint32_t m_bufferCount;

	VkRenderPassBeginInfo m_renderPassInfo;
};
#endif
