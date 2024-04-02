#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <StagingBufferManager.hpp>

namespace Constants
{
	constexpr const char* appName  = "TerraTest";
}

class StagingBufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void StagingBufferTest::SetUpTestSuite()
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

void StagingBufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(StagingBufferTest, StagingTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };
	TerraDispatcher dispatcher{};

	const VkQueueFamilyMananger& queueFamilyMan = s_deviceManager->GetQueueFamilyManager();

	VkCommandQueue transferQueue{
		logicalDevice,
		queueFamilyMan.GetQueue(QueueType::TransferQueue),
		queueFamilyMan.GetIndex(QueueType::TransferQueue)
	};
	transferQueue.CreateCommandBuffers(1u);
	VkGraphicsQueue graphicsQueue{
		logicalDevice,
		queueFamilyMan.GetQueue(QueueType::GraphicsQueue),
		queueFamilyMan.GetIndex(QueueType::GraphicsQueue),
		&dispatcher
	};
	graphicsQueue.CreateCommandBuffers(2u);

	ThreadPool threadPool{ 8u };

	StagingBufferManager stagingBufferMan{
		logicalDevice, &memoryManager, &dispatcher, &transferQueue, &threadPool
	};

	Buffer testStorage{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testStorage.Create(
		2_KB,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		{});

	VkTextureView testTextureView{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testTextureView.CreateView2D(
		1280u, 720u, VK_FORMAT_R8G8B8A8_SRGB,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, {}
	);

	std::unique_ptr<std::uint8_t> bufferData  = std::unique_ptr<std::uint8_t>{ new std::uint8_t[2_KB] };
	std::unique_ptr<std::uint8_t> textureData =
		std::unique_ptr<std::uint8_t>{ new std::uint8_t[testTextureView.GetTexture().Size()] };

	stagingBufferMan.AddBuffer(bufferData.get(), 2_KB, testStorage, 0u);
	stagingBufferMan.AddTextureView(
		textureData.get(), testTextureView.GetTexture().Size(), testTextureView, {}
	);

	VKCommandBuffer& transferCmdBuffer = transferQueue.GetCommandBuffer(0u);
	transferCmdBuffer.Reset();

	stagingBufferMan.Copy(0u);

	transferCmdBuffer.Close();

	VKFence waitFence{ logicalDevice };
	waitFence.Create(false);

	transferQueue.SubmitCommandBuffer(0u, waitFence);
	waitFence.Wait();

	stagingBufferMan.CleanUpTempBuffers();
}

