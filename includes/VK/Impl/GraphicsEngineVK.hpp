#ifndef __GRAPHICS_ENGINE_VK_HPP__
#define __GRAPHICS_ENGINE_VK_HPP__
#include <GraphicsEngine.hpp>
#include <string>
#include <vulkan/vulkan.hpp>

class GraphicsEngineVK : public GraphicsEngine {
public:
	GraphicsEngineVK(
		const char* appName,
		void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height,
		std::uint8_t bufferCount
	);
	~GraphicsEngineVK() noexcept;

	void SetBackgroundColor(const Ceres::VectorF32& colorVector) noexcept override;
	void SubmitModel(const IModel* const modelRef, bool texture = true) override;
	void Render() override;
	void Resize(std::uint32_t width, std::uint32_t height) override;
	void GetMonitorCoordinates(
		std::uint64_t& monitorWidth, std::uint64_t& monitorHeight
	) override;
	void WaitForAsyncTasks() override;

	void SetShaderPath(const char* path) noexcept override;
	void InitResourceBasedObjects() override;

private:
	void SetScissorAndViewport(std::uint32_t width, std::uint32_t height) noexcept;

private:
	VkClearColorValue m_backgroundColor;
	const std::string m_appName;

	VkViewport m_viewport;
	VkRect2D m_scissorRect;

	std::string m_shaderPath;
};
#endif
