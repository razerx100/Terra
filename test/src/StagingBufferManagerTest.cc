#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkStagingBufferManager.hpp>

using namespace Terra;

namespace Constants
{
	constexpr const char* appName      = "TerraTest";
	constexpr std::uint32_t frameCount = 2u;
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
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::StandardError);
	s_instanceManager->CreateInstance(coreVersion);
	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	{
		VkDeviceExtensionManager& extensionManager = s_deviceManager->ExtensionManager();
		extensionManager.AddExtensions(MemoryManager::GetRequiredExtensions());
	}

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

	const VkQueueFamilyMananger& queueFamilyMan = s_deviceManager->GetQueueFamilyManager();

	VkCommandQueue transferQueue{
		logicalDevice,
		queueFamilyMan.GetQueue(QueueType::TransferQueue),
		queueFamilyMan.GetIndex(QueueType::TransferQueue)
	};
	transferQueue.CreateCommandBuffers(1u);

	ThreadPool threadPool{ 8u };

	StagingBufferManager stagingBufferMan{
		logicalDevice, &memoryManager, &threadPool, s_deviceManager->GetQueueFamilyManagerRef()
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

	auto bufferData                      = std::make_unique<std::uint8_t[]>(2_KB);
	const VkDeviceSize textureBufferSize = testTextureView.GetTexture().GetBufferSize();

	auto textureData                     = std::make_unique<std::uint8_t[]>(textureBufferSize);

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	stagingBufferMan.AddBuffer(std::move(bufferData), 2_KB, &testStorage, 0u, tempDataBuffer);
	stagingBufferMan.AddTextureView(std::move(textureData), &testTextureView, {}, tempDataBuffer);

	{
		const size_t bufferIndex = 0u;
		const CommandBufferScope cmdBufferScope{ transferQueue.GetCommandBuffer(bufferIndex) };

		stagingBufferMan.CopyAndClearQueuedBuffers(cmdBufferScope);
	}

	VKFence waitFence{ logicalDevice };
	waitFence.Create(false);

	transferQueue.SubmitCommandBuffer(0u, waitFence);
	waitFence.Wait();
}

