#ifndef TERRA_HPP_
#define TERRA_HPP_
#include <ThreadPool.hpp>
#include <Renderer.hpp>
#include <VkCommandQueue.hpp>
#include <VkSyncObjects.hpp>
#include <VkDeviceManager.hpp>
#include <VKInstanceManager.hpp>
#include <SurfaceManager.hpp>
#include <SwapchainManager.hpp>
#include <IDisplayManager.hpp>
#include <DescriptorSetManager.hpp>
#include <TextureStorage.hpp>
#include <memory>
#include <DeviceMemory.hpp>
#include <UploadContainer.hpp>
#include <ObjectManager.hpp>
#include <VkHelperFunctions.hpp>

#include <RenderEngine.hpp>
#include <BufferManager.hpp>
#include <CameraManager.hpp>

class Terra
{
public:
	class Resources;
	class Queue;

public:
	Terra(
		std::string_view appName, void* windowHandle, void* moduleHandle, std::uint32_t bufferCount,
		std::uint32_t width, std::uint32_t height,
		ThreadPool& threadPool, RenderEngineType engineType
	);

	Terra(const Terra&) = delete;
	Terra& operator=(const Terra&) = delete;

	Terra(Terra&& other) noexcept;
	Terra& operator=(Terra&& other) noexcept;

	static Terra& Get();
	static void Init(
		std::string_view appName, void* windowHandle, void* moduleHandle, std::uint32_t bufferCount,
		std::uint32_t width, std::uint32_t height,
		ThreadPool& threadPool,	RenderEngineType engineType
	);

	inline IDisplayManager& Display() noexcept { return *m_display; }
	inline VkInstanceManager& Instance() noexcept { return *m_vkInstance; }
	inline SurfaceManager& Surface() noexcept { return *m_surface; }
	inline VkDeviceManager& Device() noexcept { return *m_device; }
	inline Resources& Res() noexcept { return m_res; }
	inline Queue& Graphics() noexcept { return m_graphicsQueue; }
	inline Queue& Compute() noexcept { return m_computeQueue; }
	inline Queue& Transfer() noexcept { return m_transferQueue; }
	inline SwapchainManager& Swapchain() noexcept { return *m_swapChain; }
	inline DescriptorSetManager& GraphicsDesc() noexcept { return *m_graphicsDescriptorSet; }
	inline DescriptorSetManager& ComputeDesc() noexcept { return *m_computeDescriptorSet; }
	inline RenderEngine& Engine() noexcept { return *m_renderEngine; }
	inline TextureStorage& Texture() noexcept { return *m_textureStorage; }
	inline BufferManager& Buffers() noexcept { return *m_bufferManager; }
	inline CameraManager& Camera() noexcept { return *m_cameraManager; }

	[[nodiscard]]
	inline std::string_view Name() const noexcept { return m_appName; }

private:
	void InitDisplay();
	void InitSurface();
	void InitQueues(
		VkDevice device, std::uint32_t bufferCount, const VkQueueFamilyMananger& queFamily
	);
	void InitRenderEngine(
		VkDevice device, RenderEngineType engineType, std::uint32_t bufferCount,
		QueueIndices3 queueIndices
	);

public:
	class Resources
	{
	public:
		Resources();

		Resources(const Resources&) = delete;
		Resources& operator=(const Resources&) = delete;

		inline Resources(Resources&& other) noexcept
			: m_gpuOnlyMemory{ std::move(other.m_gpuOnlyMemory) },
			m_uploadMemory{ std::move(other.m_uploadMemory) },
			m_cpuWriteMemory{ std::move(other.m_cpuWriteMemory) },
			m_uploadContainer{ std::move(other.m_uploadContainer) }
		{}

		inline Resources& operator=(Resources&& other) noexcept
		{
			m_gpuOnlyMemory   = std::move(other.m_gpuOnlyMemory);
			m_uploadMemory	  = std::move(other.m_uploadMemory);
			m_cpuWriteMemory  = std::move(other.m_cpuWriteMemory);
			m_uploadContainer = std::move(other.m_uploadContainer);

			return *this;
		}

		inline DeviceMemory& GPU() noexcept { return *m_gpuOnlyMemory; }
		inline DeviceMemory& CPU() noexcept { return *m_cpuWriteMemory; }
		inline DeviceMemory& Upload() noexcept { return *m_uploadMemory; }
		inline UploadContainer& UploadCont()  noexcept { return *m_uploadContainer; }

		void Init(
			ObjectManager& om, VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
			ThreadPool& threadPool
		);
		inline void ResetUpload() noexcept
		{
			m_uploadContainer.reset();
			m_uploadMemory.reset();
		}

	private:
		std::unique_ptr<DeviceMemory>    m_gpuOnlyMemory;
		std::unique_ptr<DeviceMemory>    m_uploadMemory;
		std::unique_ptr<DeviceMemory>    m_cpuWriteMemory;
		std::unique_ptr<UploadContainer> m_uploadContainer;
	};

	class Queue
	{
	public:
		Queue();

		Queue(const Queue&) = delete;
		Queue& operator=(const Queue&) = delete;

		inline Queue(Queue&& other) noexcept
			: m_queue{ std::move(other.m_queue) }, m_cmdBuffer{ std::move(other.m_cmdBuffer) },
			m_syncObjects{ std::move(other.m_syncObjects) } {}

		inline Queue& operator=(Queue&& other) noexcept
		{
			m_queue	      = std::move(other.m_queue);
			m_cmdBuffer	  = std::move(other.m_cmdBuffer);
			m_syncObjects = std::move(other.m_syncObjects);

			return *this;
		}

		inline VkCommandQueue& Que() noexcept { return *m_queue; }
		inline VKCommandBuffer& CmdBuffer() noexcept { return *m_cmdBuffer; }
		inline VkSyncObjects& SyncObj() noexcept { return *m_syncObjects; }

	private:
		friend void InitGraphicsQueue(
			VkQueue vkQueue, VkDevice logicalDevice, std::uint32_t queueIndex,
			std::uint32_t bufferCount, ObjectManager& om, Queue& queue
		);
		friend void InitTransferQueue(
			VkQueue vkQueue, VkDevice logicalDevice, std::uint32_t queueIndex,
			ObjectManager& om, Queue& queue
		);

	private:
		std::unique_ptr<VkCommandQueue>  m_queue;
		std::unique_ptr<VKCommandBuffer> m_cmdBuffer;
		std::unique_ptr<VkSyncObjects>   m_syncObjects;
	};

private:
	std::string_view                         m_appName;
	ObjectManager                            m_objectManager;
	std::unique_ptr<IDisplayManager>         m_display;
	std::unique_ptr<VkInstanceManager>       m_vkInstance;
	std::unique_ptr<SurfaceManager>          m_surface;
	std::unique_ptr<VkDeviceManager>         m_device;
	Resources                                m_res;
	Queue                                    m_graphicsQueue;
	Queue                                    m_computeQueue;
	Queue                                    m_transferQueue;
	std::unique_ptr<SwapchainManager>        m_swapChain;
	std::unique_ptr<DescriptorSetManager>    m_graphicsDescriptorSet;
	std::unique_ptr<DescriptorSetManager>    m_computeDescriptorSet;
	std::unique_ptr<RenderEngine>            m_renderEngine;
	std::unique_ptr<TextureStorage>          m_textureStorage;
	std::unique_ptr<BufferManager>           m_bufferManager;
	std::unique_ptr<CameraManager>           m_cameraManager;
};
#endif
