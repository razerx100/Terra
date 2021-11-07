#ifndef __GRAPHICS_ENGINE_VK_HPP__
#define __GRAPHICS_ENGINE_VK_HPP__
#include <GraphicsEngine.hpp>
#include <string>

class GraphicsEngineVK : public GraphicsEngine {
public:
	GraphicsEngineVK(
		const char* appName,
		void* windowHandle, void* moduleHandle,
		std::uint32_t width, std::uint32_t height,
		std::uint8_t bufferCount
	);
	~GraphicsEngineVK() noexcept;

	void SetBackgroundColor(Color color) noexcept override;
	void SubmitCommands() override;
	void Render() override;
	void Resize(std::uint32_t width, std::uint32_t height) override;
	SRect GetMonitorCoordinates() override;

private:
	Color m_backgroundColor;

	const std::string m_appName;
};
#endif
