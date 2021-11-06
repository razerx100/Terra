#ifndef __GRAPHICS_ENGINE_HPP__
#define __GRAPHICS_ENGINE_HPP__
#include <cstdint>

#ifdef BUILD_TERRA
#define TERRA_DLL __declspec(dllexport)
#else
#define TERRA_DLL __declspec(dllimport)
#endif

struct TERRA_DLL SRect {
	long left;
	long top;
	long right;
	long bottom;
};

struct TERRA_DLL Color {
	float r;
	float g;
	float b;
	float a;
};

class TERRA_DLL GraphicsEngine {
public:
	virtual ~GraphicsEngine() = default;

	virtual void SetBackgroundColor(Color color) noexcept = 0;
	virtual void SubmitCommands() = 0;
	virtual void Render() = 0;
	virtual void Resize(std::uint32_t width, std::uint32_t height) = 0;
	virtual SRect GetMonitorCoordinates() = 0;
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
