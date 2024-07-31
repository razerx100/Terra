#ifndef TERRA_HPP_
#define TERRA_HPP_
#include <VKInstanceManager.hpp>
#include <SurfaceManager.hpp>
#include <VkDeviceManager.hpp>
#include <SwapchainManager.hpp>
#include <DisplayManager.hpp>
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
	void Render();
	void WaitForGPUToFinish();

	// Instead of creating useless wrapper functions, let's just return a reference to the RenderEngine.
	[[nodiscard]]
	RenderEngine& GetRenderEngine() noexcept { return *m_renderEngine; }
	[[nodiscard]]
	const RenderEngine& GetRenderEngine() const noexcept { return *m_renderEngine; }

	[[nodiscard]]
	DisplayManager::Resolution GetFirstDisplayCoordinates() const;

private:
	void CreateInstance();
	void CreateSurface(void* windowHandle, void* moduleHandle);
	void CreateDevice(RenderEngineType engineType);
	void CreateDisplayManager();
	void CreateSwapchain(std::uint32_t frameCount);
	void CreateRenderEngine(
		RenderEngineType engineType, std::shared_ptr<ThreadPool>&& threadPool, std::uint32_t frameCount
	);

private:
	VkInstanceManager                 m_instanceManager;
	std::unique_ptr<SurfaceManager>   m_surfaceManager;
	VkDeviceManager                   m_deviceManager;
	std::unique_ptr<DisplayManager>   m_displayManager;
	std::unique_ptr<SwapchainManager> m_swapchain;
	std::unique_ptr<RenderEngine>     m_renderEngine;
	std::uint32_t                     m_windowWidth;
	std::uint32_t                     m_windowHeight;

	static constexpr CoreVersion s_coreVersion = CoreVersion::V1_3;

public:
	Terra(const Terra&) = delete;
	Terra& operator=(const Terra&) = delete;

	Terra(Terra&& other) noexcept
		: m_instanceManager{ std::move(other.m_instanceManager) },
		m_surfaceManager{ std::move(other.m_surfaceManager) },
		m_deviceManager{ std::move(other.m_deviceManager) },
		m_displayManager{ std::move(other.m_displayManager) },
		m_swapchain{ std::move(other.m_swapchain) },
		m_renderEngine{ std::move(other.m_renderEngine) },
		m_windowWidth{ other.m_windowWidth },
		m_windowHeight{ other.m_windowHeight }
	{}
	Terra& operator=(Terra&& other) noexcept
	{
		m_instanceManager = std::move(other.m_instanceManager);
		m_surfaceManager  = std::move(other.m_surfaceManager);
		m_deviceManager   = std::move(other.m_deviceManager);
		m_displayManager  = std::move(other.m_displayManager);
		m_swapchain       = std::move(other.m_swapchain);
		m_renderEngine    = std::move(other.m_renderEngine);
		m_windowWidth     = other.m_windowWidth;
		m_windowHeight    = other.m_windowHeight;

		return *this;
	}
};
#endif
