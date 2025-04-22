#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkRenderEngineVS.hpp>
#include <VkRenderEngineMS.hpp>

namespace Constants
{
	constexpr const char* appName      = "TerraTest";
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
