#include <GraphicsEngineVK.hpp>

static GraphicsEngine* s_pGraphicsEngine = nullptr;

GraphicsEngine* GetGraphicsEngineInstance() noexcept {
	return s_pGraphicsEngine;
}

void InitGraphicsEngineInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) {
	if (!s_pGraphicsEngine)
		s_pGraphicsEngine = new GraphicsEngineVK(
			appName,
			windowHandle, moduleHandle, width, height, bufferCount
		);
}

void CleanUpGraphicsEngineInstance() noexcept {
	if (s_pGraphicsEngine)
		delete s_pGraphicsEngine;
}