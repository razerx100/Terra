#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <ISurfaceManager.hpp>
#ifdef TERRA_WIN32
#include <SurfaceManagerWin32.hpp>
#include <SimpleWindow.hpp>
#endif
#include <VKRenderPass.hpp>
#include <SwapChainManager.hpp>

namespace Constants
{
	constexpr std::uint32_t width  = 1920u;
	constexpr std::uint32_t height = 1080u;
	constexpr const char* appName  = "TerraTest";
}

class SwapchainTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<ISurfaceManager>   s_surfaceManager;
#ifdef TERRA_WIN32
	inline static std::unique_ptr<SimpleWindow>      s_window;
#endif
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
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
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::standardError);

	{
		VkInstanceExtensionManager& extensionManager = s_instanceManager->ExtensionManager();

#ifdef TERRA_WIN32
		extensionManager.AddExtensions(s_surfaceManager->GetRequiredExtensions());
#endif
	}

	s_instanceManager->CreateInstance(coreVersion);

	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	VkSurfaceKHR surface = VK_NULL_HANDLE;

#ifdef TERRA_WIN32
	s_surfaceManager->Create(vkInstance, s_window->GetWindowHandle(), s_window->GetModuleInstance());
	surface = s_surfaceManager->Get();
#endif

	s_deviceManager = std::make_unique<VkDeviceManager>();

	{
		VkDeviceExtensionManager& extensionManager = s_deviceManager->ExtensionManager();

		extensionManager.AddExtensions(SwapChainManager::GetRequiredExtensions());
	}

	s_deviceManager->FindPhysicalDevice(vkInstance, surface).CreateLogicalDevice(coreVersion);
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
