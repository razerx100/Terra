#ifndef TERRA_HPP_
#define TERRA_HPP_
#include <VKInstanceManager.hpp>
#include <SurfaceManager.hpp>
#include <VkDeviceManager.hpp>
#include <SwapchainManager.hpp>
#include <RendererTypes.hpp>
#include <RenderEngine.hpp>

class Terra
{
public:
	Terra(
		std::string_view appName,
		void* windowHandle, void* moduleHandle, std::uint32_t width, std::uint32_t height,
		std::uint32_t bufferCount, std::shared_ptr<ThreadPool> threadPool, RenderEngineType engineType
	);

	void Resize(std::uint32_t width, std::uint32_t height);

private:
	void CreateInstance();
	void CreateSurface(void* windowHandle, void* moduleHandle);
	void CreateDevice(RenderEngineType engineType);
	void CreateSwapchain(std::uint32_t frameCount);
	void CreateRenderEngine(
		RenderEngineType engineType, std::shared_ptr<ThreadPool>&& threadPool, std::uint32_t frameCount
	);

private:
	VkInstanceManager                 m_instanceManager;
	std::unique_ptr<SurfaceManager>   m_surfaceManager;
	VkDeviceManager                   m_deviceManager;
	std::unique_ptr<SwapchainManager> m_swapchain;
	std::unique_ptr<RenderEngine>     m_renderEngine;

	static constexpr CoreVersion s_coreVersion = CoreVersion::V1_3;

public:
	Terra(const Terra&) = delete;
	Terra& operator=(const Terra&) = delete;

	Terra(Terra&& other) noexcept
		: m_instanceManager{ std::move(other.m_instanceManager) },
		m_surfaceManager{ std::move(other.m_surfaceManager) },
		m_deviceManager{ std::move(other.m_deviceManager) },
		m_swapchain{ std::move(other.m_swapchain) },
		m_renderEngine{ std::move(other.m_renderEngine) }
	{}
	Terra& operator=(Terra&& other) noexcept
	{
		m_instanceManager = std::move(other.m_instanceManager);
		m_surfaceManager  = std::move(other.m_surfaceManager);
		m_deviceManager   = std::move(other.m_deviceManager);
		m_swapchain       = std::move(other.m_swapchain);
		m_renderEngine    = std::move(other.m_renderEngine);

		return *this;
	}
};
#endif
