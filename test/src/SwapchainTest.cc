#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkSurfaceManager.hpp>
#ifdef TERRA_WIN32
#include <VkSurfaceManagerWin32.hpp>
#include <SimpleWindow.hpp>
#endif
#include <VKRenderPass.hpp>
#include <VkSwapchainManager.hpp>

namespace Constants
{
	constexpr std::uint32_t width       = 1920;
	constexpr std::uint32_t height      = 1080u;
	constexpr std::uint32_t bufferCount = 2u;
	constexpr const char* appName       = "TerraTest";
}

class SwapchainTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager>   s_instanceManager;
#ifdef TERRA_WIN32
	inline static std::unique_ptr<SurfaceManagerWin32> s_surfaceManager;
	inline static std::unique_ptr<SimpleWindow>        s_window;
#endif
	inline static std::unique_ptr<VkDeviceManager>     s_deviceManager;
};

void SwapchainTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

#ifdef TERRA_WIN32
	s_window         = std::make_unique<SimpleWindow>(
		Constants::width, Constants::height, Constants::appName
	);
	s_surfaceManager = std::make_unique<SurfaceManagerWin32>();
#endif

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::StandardError);

	{
		VkInstanceExtensionManager& extensionManager = s_instanceManager->ExtensionManager();

#ifdef TERRA_WIN32
		SurfaceInstanceExtensionWin32::SetInstanceExtensions(extensionManager);
#endif
	}

	s_instanceManager->CreateInstance(coreVersion);

	VkInstance vkInstance = s_instanceManager->GetVKInstance();

#ifdef TERRA_WIN32
	s_surfaceManager->Create(vkInstance, s_window->GetWindowHandle(), s_window->GetModuleInstance());
#endif

	s_deviceManager = std::make_unique<VkDeviceManager>();

	{
		VkDeviceExtensionManager& extensionManager = s_deviceManager->ExtensionManager();

		extensionManager.AddExtensions(SwapchainManager::GetRequiredExtensions());
		extensionManager.AddExtensions(MemoryManager::GetRequiredExtensions());
	}

	s_deviceManager->SetDeviceFeatures(coreVersion)
		.SetPhysicalDeviceAutomatic(vkInstance, s_surfaceManager->Get())
		.CreateLogicalDevice();
}

void SwapchainTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_surfaceManager.reset();
	s_instanceManager.reset();
}

TEST_F(SwapchainTest, RenderPassTest)
{
	VkDevice logicalDevice = s_deviceManager->GetLogicalDevice();

	VKRenderPass renderPass1{ logicalDevice };

	{
		VKRenderPass renderPass{ logicalDevice };
		renderPass.Create(
			RenderPassBuilder{}
			.AddColourAttachment(VK_FORMAT_R8G8B8A8_SRGB)
			.AddDepthAttachment(VK_FORMAT_D32_SFLOAT)
			.Build()
		);

		renderPass1 = std::move(renderPass);
	}

	EXPECT_NE(renderPass1.Get(), VK_NULL_HANDLE) << "RenderPass is null.";
}

TEST_F(SwapchainTest, SwapchainManagerTest)
{
	VkDeviceManager& device = *s_deviceManager;

	VkDevice logicalDevice          = device.GetLogicalDevice();
	VkPhysicalDevice physicalDevice = device.GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 100_MB, 20_MB };

	VKRenderPass renderPass{ logicalDevice };

	SwapchainManager swapchain{
		logicalDevice, device.GetQueueFamilyManager().GetQueue(QueueType::GraphicsQueue),
		Constants::bufferCount
	};

	swapchain.CreateSwapchain(
		s_deviceManager->GetPhysicalDevice(), *s_surfaceManager, Constants::width, Constants::height
	);

	{
		const VkExtent2D newExtent = swapchain.GetCurrentSwapchainExtent();

		renderPass.Create(
			RenderPassBuilder{}
			.AddColourAttachment(swapchain.GetSwapchainFormat())
			.Build()
		);
	}

	EXPECT_NE(swapchain.GetVkSwapchain(), VK_NULL_HANDLE) << "Swapchain creation failed.";

	s_window->SetWindowResolution(2560u, 1440u);

	swapchain.CreateSwapchain(s_deviceManager->GetPhysicalDevice(), *s_surfaceManager, 2560u, 1440u);

	{
		const VkExtent2D newExtent = swapchain.GetCurrentSwapchainExtent();

		renderPass.Create(
			RenderPassBuilder{}
			.AddColourAttachment(swapchain.GetSwapchainFormat())
			.Build()
		);
	}

	EXPECT_NE(swapchain.GetVkSwapchain(), VK_NULL_HANDLE) << "Swapchain re-creation failed.";
}
