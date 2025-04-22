#include <TerraInstance.hpp>

#include <RendererVK.hpp>

std::unique_ptr<Renderer> CreateTerraInstance(
	const char* appName, void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height, std::shared_ptr<ThreadPool> threadPool,
	RenderEngineType engineType, std::uint32_t bufferCount
) {
	using namespace Terra;

#ifdef TERRA_WIN32
	if (engineType == RenderEngineType::MeshDraw)
	{
		using Renderer_t
			= RendererVK<SurfaceManagerWin32, DisplayManagerWin32, RenderEngineMS>;

		return std::make_unique<Renderer_t>(
			appName, windowHandle, moduleHandle, width, height, bufferCount, std::move(threadPool)
		);
	}
	else if (engineType == RenderEngineType::IndirectDraw)
	{
		using Renderer_t
			= RendererVK<SurfaceManagerWin32, DisplayManagerWin32, RenderEngineVSIndirect>;

		return std::make_unique<Renderer_t>(
			appName, windowHandle, moduleHandle, width, height, bufferCount, std::move(threadPool)
		);
	}
	else
	{
		using Renderer_t
			= RendererVK<SurfaceManagerWin32, DisplayManagerWin32, RenderEngineVSIndividual>;

		return std::make_unique<Renderer_t>(
			appName, windowHandle, moduleHandle, width, height, bufferCount, std::move(threadPool)
		);
	}
#endif
}
