#ifndef __TERRA_INSTANCE_HPP__
#define __TERRA_INSTANCE_HPP__
#include <IGraphicsEngine.hpp>

#ifdef BUILD_TERRA
#define TERRA_DLL __declspec(dllexport)
#else
#define TERRA_DLL __declspec(dllimport)
#endif

TERRA_DLL GraphicsEngine* __cdecl CreateTerraInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	size_t bufferCount = 2u
);
#endif
