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

	[[nodiscard]]
	Resolution GetDisplayCoordinates(std::uint32_t displayIndex = 0u) const override;

	[[nodiscard]]
	size_t RegisterResource(
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
	) override;

	void SetThreadPool(std::shared_ptr<IThreadPool> threadPoolArg) noexcept override;
	void SetBackgroundColour(const std::array<float, 4>& colourVector) noexcept override;
	void SubmitModel(const IModel* const modelRef) override;
	void Render() override;
	void WaitForAsyncTasks() override;

	void SetShaderPath(const char* path) noexcept override;
	void InitResourceBasedObjects() override;
	void ProcessData() override;

	void SetSharedDataContainer(
		std::shared_ptr<class ISharedDataContainer> sharedData
	) noexcept override;

private:
	VkClearColorValue m_backgroundColour;
	const std::string m_appName;

	std::string m_shaderPath;
	std::uint32_t m_bufferCount;
	std::uint32_t m_width;
	std::uint32_t m_height;
};
#endif
