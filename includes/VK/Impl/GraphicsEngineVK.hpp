#ifndef __GRAPHICS_ENGINE_VK_HPP__
#define __GRAPHICS_ENGINE_VK_HPP__
#include <GraphicsEngine.hpp>
#include <vulkan/vulkan.hpp>
#include <string>

class GraphicsEngineVK : public GraphicsEngine {
public:
	GraphicsEngineVK(
		const char* appName,
		void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height,
		size_t bufferCount
	);
	~GraphicsEngineVK() noexcept;

	[[nodiscard]]
	size_t RegisterResource(const void* data, size_t size, bool texture = true) override;

	void SetBackgroundColor(const Ceres::Float32_4& colorVector) noexcept override;
	void SubmitModel(const IModel* const modelRef, bool texture = true) override;
	void Render() override;
	void Resize(std::uint32_t width, std::uint32_t height) override;
	void GetMonitorCoordinates(
		std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
	) override;
	void WaitForAsyncTasks() override;

	void SetShaderPath(const char* path) noexcept override;
	void InitResourceBasedObjects() override;
	void ProcessData() override;

private:
	VkClearValue m_backgroundColor;
	const std::string m_appName;

	std::string m_shaderPath;
	size_t m_bufferCount;

	VkRenderPassBeginInfo m_renderPassInfo;
};
#endif
