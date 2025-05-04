#include <gtest/gtest.h>
#include <memory>
#include <limits>
#include <ranges>
#include <algorithm>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>

#include <VkExternalRenderPass.hpp>
#include <VkExternalBuffer.hpp>

using namespace Terra;

namespace Constants
{
	constexpr const char* appName      = "TerraTest";
	constexpr std::uint32_t frameCount = 2u;
}

class ExternalResourceTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void ExternalResourceTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::StandardError);
	s_instanceManager->CreateInstance(coreVersion);

	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	s_deviceManager->SetDeviceFeatures(coreVersion)
		.SetPhysicalDeviceAutomatic(vkInstance)
		.CreateLogicalDevice();
}

void ExternalResourceTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(ExternalResourceTest, VkExternalRenderPassTest)
{
	auto vkRenderPass = std::make_shared<VkExternalRenderPass>();

	ExternalRenderPass<VkExternalRenderPass> renderPass{ vkRenderPass };
}

TEST_F(ExternalResourceTest, VkExternalBufferTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	auto vkBuffer = std::make_shared<VkExternalBuffer>(
		logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	);

	ExternalBuffer<VkExternalBuffer> buffer{ vkBuffer };
}

TEST_F(ExternalResourceTest, VkExternalTextureTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	auto vkTexture = std::make_shared<VkExternalTexture>(logicalDevice, &memoryManager);

	ExternalTexture<VkExternalTexture> texture{ vkTexture };
}
