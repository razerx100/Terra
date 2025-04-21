#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkSharedBuffers.hpp>

namespace Constants
{
	constexpr const char* appName  = "TerraTest";
}

class VkSharedBufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void VkSharedBufferTest::SetUpTestSuite()
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

void VkSharedBufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(VkSharedBufferTest, SharedBufferTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	SharedBufferGPU sharedBuffer{ logicalDevice, &memoryManager, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, {} };

	Callisto::TemporaryDataBufferGPU tempDataBuffer{};

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 0u) << "Offset isn't 0.";
		EXPECT_EQ(size, 12u) << "Size isn't 12Bytes.";
	}

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 12u) << "Offset isn't 12.";
		EXPECT_EQ(size, 24u) << "Size isn't 24Bytes.";

		sharedBuffer.RelinquishMemory(allocInfo);
	}

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 12u) << "Offset isn't 12.";
		EXPECT_EQ(size, 24u) << "Size isn't 24Bytes.";
	}

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(12u, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 24u) << "Offset isn't 12.";
		EXPECT_EQ(size, 36u) << "Size isn't 36Bytes.";
	}

	const size_t midOffset = 36u;

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(20_KB, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 0u + midOffset) << "Offset isn't 0.";
		EXPECT_EQ(size, 20_KB + midOffset) << "Size isn't 20KB.";
	}

	// Need to do a GPU copy inbetween.
	// sharedBuffer.CopyOldBuffer(copyBuffer);
	// tempDataBuffer.SetUsed();

	auto twentyKBAllocInfo = sharedBuffer.AllocateAndGetSharedData(30_KB, tempDataBuffer);
	{
		VkDeviceSize offset = twentyKBAllocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 20_KB + midOffset) << "Offset isn't 20KB.";
		EXPECT_EQ(size, 20_KB + 30_KB + midOffset) << "Size isn't 50KB.";
	}

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(50_KB, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 50_KB + midOffset) << "Offset isn't 50KB.";
		EXPECT_EQ(size, 100_KB + midOffset) << "Size isn't 100KB.";
	}

	sharedBuffer.RelinquishMemory(twentyKBAllocInfo);

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(20_KB, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 20_KB + midOffset) << "Offset isn't 20KB.";
		EXPECT_EQ(size, 100_KB + midOffset) << "Size isn't 100KB.";
	}

	{
		auto allocInfo      = sharedBuffer.AllocateAndGetSharedData(10_KB, tempDataBuffer);
		VkDeviceSize offset = allocInfo.offset;
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 40_KB + midOffset) << "Offset isn't 40KB.";
		EXPECT_EQ(size, 100_KB + midOffset) << "Size isn't 100KB.";
	}
}
