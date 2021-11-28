#ifndef __GRAPHICS_ENGINE_HPP__
#define __GRAPHICS_ENGINE_HPP__
#include <cstdint>

#ifdef BUILD_TERRA
#define TERRA_DLL __declspec(dllexport)
#else
#define TERRA_DLL __declspec(dllimport)
#endif

class TERRA_DLL GraphicsEngine {
public:
	virtual ~GraphicsEngine() = default;

	virtual void SetBackgroundColor(const float* colorVector) noexcept = 0;
	virtual void SubmitModels(const class IModel* const models, std::uint32_t modelCount) = 0;
	virtual void Render() = 0;
	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
	virtual void GetMonitorCoordinates(std::uint64_t* rectCoord) = 0;
	virtual void WaitForAsyncTasks() = 0;
};

TERRA_DLL GraphicsEngine* __cdecl GetGraphicsEngineInstance() noexcept;
TERRA_DLL void __cdecl InitGraphicsEngineInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount = 2u
);
TERRA_DLL void __cdecl CleanUpGraphicsEngineInstance() noexcept;

#endif
