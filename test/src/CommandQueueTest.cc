#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkCommandQueue.hpp>
#include <VkSyncObjects.hpp>
#include <VkTextureView.hpp>

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
	queue.CreateCommandBuffers(Constants::bufferCount);

	{
		type = QueueType::ComputeQueue;

		VkCommandQueue queue1{ logicalDevice, queFamilyMan.GetQueue(type), queFamilyMan.GetIndex(type) };
		queue1.CreateCommandBuffers(Constants::bufferCount);

		queue = std::move(queue1);
	}

	EXPECT_NE(queue.GetCommandBuffer(1u).Get(), VK_NULL_HANDLE) << "Command buffer creation failed.";

	VKFence fence{ logicalDevice };
	fence.Create(false);

	queue.GetCommandBuffer(0u).Reset();
	queue.GetCommandBuffer(0u).Close();

	queue.SubmitCommandBuffer(0u, fence);

	fence.Wait();
}

TEST_F(CommandQueueTest, CommandQueueCopyTest)
{
	VkDevice logicalDevice                    = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice           = s_deviceManager->GetPhysicalDevice();
	const VkQueueFamilyMananger& queFamilyMan = s_deviceManager->GetQueueFamilyManager();

	const QueueType type = QueueType::TransferQueue;

	VkCommandQueue queue{ logicalDevice, queFamilyMan.GetQueue(type), queFamilyMan.GetIndex(type) };
	queue.CreateCommandBuffers(1u);

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 200_MB, 200_KB };

	Buffer testBuffer1{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
	testBuffer1.Create(
		20_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		{}
	);

	Buffer testBuffer2{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testBuffer2.Create(
		20_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		{}
	);

	VkTextureView testTextureView{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testTextureView.CreateView2D(
		1280u, 720u, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, {}
	);

	Buffer testBuffer3{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
	testBuffer3.Create(
		testTextureView.GetTexture().Size(),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, {}
	);

	{
		VKCommandBuffer& cmdBuffer = queue.GetCommandBuffer(0u);
		cmdBuffer.Reset();

		cmdBuffer.Copy(testBuffer1, testBuffer2);
		cmdBuffer.Copy(testBuffer3, testTextureView);

		cmdBuffer.Close();
	}

	VKFence fence{ logicalDevice };
	fence.Create(false);

	queue.SubmitCommandBuffer(0u, fence);

	fence.Wait();
}
