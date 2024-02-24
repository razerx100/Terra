#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VKRenderPass.hpp>

namespace Constants
{
	constexpr const char* appName  = "TerraTest";
}

class SwapchainTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void SwapchainTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::standardError);
	s_instanceManager->CreateInstance(coreVersion);

	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	// TODO: Add a valid surface for testing. The validation error is because of the null surface.
	s_deviceManager->FindPhysicalDevice(vkInstance, VK_NULL_HANDLE).CreateLogicalDevice(coreVersion);
}

void SwapchainTest::TearDownTestSuite()
{
	s_deviceManager.reset();
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
