#include <TerraInstance.hpp>

#include <RendererVK.hpp>

std::shared_ptr<Renderer> CreateTerraInstance(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height, std::shared_ptr<ThreadPool> threadPool,
	RenderEngineType engineType, std::uint32_t bufferCount
) {
	return std::make_shared<RendererVK>(
		appName, windowHandle, moduleHandle, width, height, bufferCount, std::move(threadPool), engineType
	);
}
