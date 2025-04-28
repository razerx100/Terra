#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkRenderEngineVS.hpp>
#include <VkRenderEngineMS.hpp>

#ifdef TERRA_WIN32
#include <SimpleWindow.hpp>
#endif

#include <RendererVK.hpp>

using namespace Terra;

namespace Constants
{
	constexpr const char* appName      = "TerraTest";
	constexpr std::uint32_t width      = 1920;
	constexpr std::uint32_t height     = 1080u;
	constexpr std::uint32_t frameCount = 2u;
	constexpr CoreVersion coreVersion  = CoreVersion::V1_3;
}

class RenderEngineTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
};

void RenderEngineTest::SetUpTestSuite()
{
	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::StandardError);
	s_instanceManager->CreateInstance(Constants::coreVersion);
}

void RenderEngineTest::TearDownTestSuite()
{
	s_instanceManager.reset();
}

TEST_F(RenderEngineTest, RenderEngineVSIndividualTest)
{
	VkDeviceManager deviceManager{};

	{
		VkDeviceExtensionManager& extensionManager = deviceManager.ExtensionManager();
		RenderEngineVSIndividualDeviceExtension::SetDeviceExtensions(extensionManager);
	}

	{
		VkInstance vkInstance = s_instanceManager->GetVKInstance();

		deviceManager.SetDeviceFeatures(Constants::coreVersion)
			.SetPhysicalDeviceAutomatic(vkInstance)
			.CreateLogicalDevice();
	}

	auto threadPool = std::make_shared<ThreadPool>(2u);

	RenderEngineVSIndividual renderEngine{ deviceManager, threadPool, Constants::frameCount };
}

TEST_F(RenderEngineTest, RenderEngineVSIndirectTest)
{
	VkDeviceManager deviceManager{};

	{
		VkDeviceExtensionManager& extensionManager = deviceManager.ExtensionManager();
		RenderEngineVSIndirectDeviceExtension::SetDeviceExtensions(extensionManager);
	}

	{
		VkInstance vkInstance = s_instanceManager->GetVKInstance();

		deviceManager.SetDeviceFeatures(Constants::coreVersion)
			.SetPhysicalDeviceAutomatic(vkInstance)
			.CreateLogicalDevice();
	}

	auto threadPool = std::make_shared<ThreadPool>(2u);

	RenderEngineVSIndirect renderEngine{ deviceManager, threadPool, Constants::frameCount };
}

TEST_F(RenderEngineTest, RenderEngineMSTest)
{
	VkDeviceManager deviceManager{};

	{
		VkDeviceExtensionManager& extensionManager = deviceManager.ExtensionManager();
		RenderEngineMSDeviceExtension::SetDeviceExtensions(extensionManager);
	}

	{
		VkInstance vkInstance = s_instanceManager->GetVKInstance();

		deviceManager.SetDeviceFeatures(Constants::coreVersion)
			.SetPhysicalDeviceAutomatic(vkInstance)
			.CreateLogicalDevice();
	}

	auto threadPool = std::make_shared<ThreadPool>(2u);

	RenderEngineMS renderEngine{ deviceManager, threadPool, Constants::frameCount };
}

TEST(RendererVKTest, RendererTest)
{
#ifdef TERRA_WIN32
	SimpleWindow window{ Constants::width, Constants::height, Constants::appName };

	RendererVK<SurfaceManagerWin32, DisplayManagerWin32, RenderEngineMS> renderer{
		Constants::appName, window.GetWindowHandle(), window.GetModuleInstance(),
		Constants::width, Constants::height, Constants::frameCount,
		std::make_shared<ThreadPool>(8u)
	};
#endif
}
