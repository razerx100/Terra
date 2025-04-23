#ifndef TERRA_HPP_
#define TERRA_HPP_
#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkSwapchainManager.hpp>
#include <VkRenderEngine.hpp>
#include <VkExternalFormatMap.hpp>

#ifdef TERRA_WIN32
#include <VkDisplayManagerWin32.hpp>
#include <VkSurfaceManagerWin32.hpp>
#else
#include <VkDisplayManagerVK.hpp>
#endif

#include <VkRenderEngineVS.hpp>
#include <VkRenderEngineMS.hpp>

namespace Terra
{
template<class SurfaceManager_t, class DisplayManager_t, class RenderEngine_t>
class Terra
{
public:
	Terra(
		std::string_view appName,
		void* windowHandle, void* moduleHandle, std::uint32_t width, std::uint32_t height,
		std::uint32_t bufferCount, std::shared_ptr<ThreadPool> threadPool
	) : m_instanceManager{ CreateInstance(std::move(appName)) },
		m_surfaceManager{
			CreateSurface(m_instanceManager.GetVKInstance(), windowHandle, moduleHandle)
		},
		m_deviceManager{
			CreateDevice(m_instanceManager.GetVKInstance(), m_surfaceManager.Get())
		},
		m_displayManager{}, m_swapchain{ CreateSwapchain(m_deviceManager, bufferCount) },
		m_renderEngine{ m_deviceManager, std::move(threadPool), bufferCount },
		// The width and height are zero initialised, as they will be set with the call to Resize.
		m_windowWidth{ 0u }, m_windowHeight{ 0u }
	{
		// Need to create the swapchain and frame buffers and stuffs.
		Resize(width, height);
	}

	void FinaliseInitialisation()
	{
		m_renderEngine.FinaliseInitialisation();
	}

	void Resize(std::uint32_t width, std::uint32_t height)
	{
		// Only recreate these if the new resolution is different.
		if (m_windowWidth != width || m_windowHeight != height)
		{
			WaitForGPUToFinish();

			// Must recreate the swapchain first.
			m_swapchain.CreateSwapchain(
				m_deviceManager.GetPhysicalDevice(), m_surfaceManager, width, height
			);

			const VkExtent2D newExtent = m_swapchain.GetCurrentSwapchainExtent();

			// Using the new extent, as it might be slightly different than the values we got.
			// For example, the title area of a window can't be drawn upon.
			// So, the actual rendering area would be a bit smaller.
			m_renderEngine.Resize(
				newExtent.width, newExtent.height, m_swapchain.HasSwapchainFormatChanged()
			);

			m_windowWidth  = width;
			m_windowHeight = height;
		}
	}
	void Render()
	{
		// Semaphores will be initialised as 0. So, the starting value should be 1.
		static std::uint64_t semaphoreCounter = 1u;

		VkDevice device = m_deviceManager.GetLogicalDevice();

		// This semaphore will be signaled when the image becomes available.
		m_swapchain.QueryNextImageIndex(device);

		const VKSemaphore& nextImageSemaphore = m_swapchain.GetNextImageSemaphore();
		const std::uint32_t nextImageIndexU32 = m_swapchain.GetNextImageIndex();
		const size_t nextImageIndex           = nextImageIndexU32;

		VkSemaphore renderFinishedSemaphore = m_renderEngine.Render(
			nextImageIndex, m_swapchain.GetColourAttachment(nextImageIndex),
			m_swapchain.GetCurrentSwapchainExtent(), semaphoreCounter, nextImageSemaphore
		);

		m_swapchain.Present(nextImageIndexU32, renderFinishedSemaphore);
	}
	void WaitForGPUToFinish()
	{
		vkDeviceWaitIdle(m_deviceManager.GetLogicalDevice());
	}

	[[nodiscard]]
	auto&& GetRenderEngine(this auto&& self) noexcept
	{
		return std::forward_like<decltype(self)>(self.m_renderEngine);
	}

	[[nodiscard]]
	ExternalFormat GetSwapchainFormat() const noexcept
	{
		return GetExternalFormat(m_swapchain.GetSwapchainFormat());
	}

	[[nodiscard]]
	VkExtent2D GetFirstDisplayCoordinates() const
	{
		return m_displayManager.GetDisplayResolution(m_deviceManager.GetPhysicalDevice(), 0u);
	}

	[[nodiscard]]
	VkExtent2D GetCurrentRenderArea() const noexcept
	{
		return m_swapchain.GetCurrentSwapchainExtent();
	}

private:
	[[nodiscard]]
	static VkInstanceManager CreateInstance(std::string_view appName)
	{
		VkInstanceManager instanceManager{ std::move(appName) };

#ifndef NDEBUG
		instanceManager.DebugLayers().AddDebugCallback(DebugCallbackType::FileOut);
#endif

		{
			VkInstanceExtensionManager& extensionManager = instanceManager.ExtensionManager();

#ifdef TERRA_WIN32
			SurfaceInstanceExtensionWin32::SetInstanceExtensions(extensionManager);

			DisplayInstanceExtensionWin32::SetInstanceExtensions(extensionManager);
#endif
		}

		instanceManager.CreateInstance(s_coreVersion);

		return instanceManager;
	}

	[[nodiscard]]
	static SurfaceManager_t CreateSurface(
		VkInstance vkInstance, void* windowHandle, void* moduleHandle
	) {
		SurfaceManager_t surfaceManager{};

		surfaceManager.Create(vkInstance, windowHandle, moduleHandle);

		return surfaceManager;
	}

	[[nodiscard]]
	static VkDeviceManager CreateDevice(VkInstance vkInstance, VkSurfaceKHR vkSurface)
	{
		VkDeviceManager deviceManager{};

		{
			VkDeviceExtensionManager& extensionManager = deviceManager.ExtensionManager();

			extensionManager.AddExtensions(SwapchainManager::GetRequiredExtensions());

			if constexpr (std::is_same_v<RenderEngine_t, RenderEngineMS>)
				RenderEngineMSDeviceExtension::SetDeviceExtensions(extensionManager);
			else if constexpr (std::is_same_v<RenderEngine_t, RenderEngineVSIndirect>)
				RenderEngineVSIndirectDeviceExtension::SetDeviceExtensions(extensionManager);
			else
				RenderEngineVSIndividualDeviceExtension::SetDeviceExtensions(extensionManager);
		}

		deviceManager.SetDeviceFeatures(s_coreVersion)
			.SetPhysicalDeviceAutomatic(vkInstance, vkSurface)
			.CreateLogicalDevice();

		return deviceManager;
	}

	[[nodiscard]]
	static SwapchainManager CreateSwapchain(
		const VkDeviceManager& deviceManager, std::uint32_t frameCount
	) {
		VkDevice device = deviceManager.GetLogicalDevice();

		return SwapchainManager{
			device, deviceManager.GetQueueFamilyManager().GetQueue(QueueType::GraphicsQueue),
			frameCount
		};
	}

private:
	VkInstanceManager m_instanceManager;
	SurfaceManager_t  m_surfaceManager;
	VkDeviceManager   m_deviceManager;
	DisplayManager_t  m_displayManager;
	SwapchainManager  m_swapchain;
	RenderEngine_t    m_renderEngine;
	std::uint32_t     m_windowWidth;
	std::uint32_t     m_windowHeight;

	static constexpr CoreVersion s_coreVersion = CoreVersion::V1_3;

public:
	// Wrappers for Render Engine functions which requires waiting for the GPU to finish first.
	[[nodiscard]]
	size_t AddTextureAsCombined(STexture&& texture)
	{
		WaitForGPUToFinish();

		return m_renderEngine.AddTextureAsCombined(std::move(texture));
	}

	[[nodiscard]]
	std::uint32_t BindCombinedTexture(size_t textureIndex)
	{
		WaitForGPUToFinish();

		return m_renderEngine.BindCombinedTexture(textureIndex);
	}

	void RemoveTexture(size_t textureIndex)
	{
		WaitForGPUToFinish();

		m_renderEngine.RemoveTexture(textureIndex);
	}

	[[nodiscard]]
	std::uint32_t AddModelBundle(std::shared_ptr<ModelBundle>&& modelBundle)
	{
		WaitForGPUToFinish();

		return m_renderEngine.AddModelBundle(std::move(modelBundle));
	}

	[[nodiscard]]
	std::uint32_t AddMeshBundle(std::unique_ptr<MeshBundleTemporary> meshBundle)
	{
		WaitForGPUToFinish();

		return m_renderEngine.AddMeshBundle(std::move(meshBundle));
	}

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
}
#endif
