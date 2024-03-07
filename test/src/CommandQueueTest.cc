#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkCommandQueue.hpp>
#include <VkSyncObjects.hpp>

namespace Constants
{
	constexpr const char* appName       = "TerraTest";
	constexpr std::uint32_t bufferCount = 2u;
}

class CommandQueueTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void CommandQueueTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::standardError);
	s_instanceManager->CreateInstance(coreVersion);
	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	s_deviceManager->SetDeviceFeatures(coreVersion)
		.SetPhysicalDeviceAutomatic(vkInstance)
		.CreateLogicalDevice();
}

void CommandQueueTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(CommandQueueTest, BasicCommandQueueTest)
{
	VkDevice logicalDevice                    = s_deviceManager->GetLogicalDevice();
	const VkQueueFamilyMananger& queFamilyMan = s_deviceManager->GetQueueFamilyManager();

	QueueType type = QueueType::GraphicsQueue;

	VkCommandQueue queue{ logicalDevice, queFamilyMan.GetQueue(type), queFamilyMan.GetIndex(type) };
	queue.CreateBuffers(Constants::bufferCount);

	EXPECT_NE(queue.Get(), VK_NULL_HANDLE) << "Queue creation failed.";

	{
		type = QueueType::ComputeQueue;

		VkCommandQueue queue1{ logicalDevice, queFamilyMan.GetQueue(type), queFamilyMan.GetIndex(type) };
		queue1.CreateBuffers(Constants::bufferCount);

		queue = std::move(queue1);
	}

	EXPECT_NE(queue.GetBuffer(1u).Get(), VK_NULL_HANDLE) << "Command buffer creation failed.";

	VKFence fence{ logicalDevice };
	fence.Create(false);

	queue.GetBuffer(0u).Reset();
	queue.GetBuffer(0u).Close();

	queue.SubmitCommandBuffer(QueueSubmitBuilder{}, 0u, fence.Get());

	fence.Wait();
}
