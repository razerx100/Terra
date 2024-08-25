#ifndef TERRA_INSTANCE_HPP_
#define TERRA_INSTANCE_HPP_
#include <Renderer.hpp>
#include <ThreadPool.hpp>
#include <memory>

#ifdef BUILD_TERRA
#define TERRA_DLL __declspec(dllexport)
#else
#define TERRA_DLL __declspec(dllimport)
#endif

TERRA_DLL std::shared_ptr<Renderer> CreateTerraInstance(
	const char* appName,
	void* windowHandle, void* moduleHandle,
	std::uint32_t width, std::uint32_t height, std::shared_ptr<ThreadPool> threadPool,
	RenderEngineType engineType, std::uint32_t bufferCount = 2u
);
#endif
