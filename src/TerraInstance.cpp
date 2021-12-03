#include <TerraInstance.hpp>
#include <GraphicsEngineVK.hpp>

GraphicsEngine* CreateTerraInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) {
	return new GraphicsEngineVK(
		appName,
		windowHandle, moduleHandle, width, height, bufferCount
	);
}
