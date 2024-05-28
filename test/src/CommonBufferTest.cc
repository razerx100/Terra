#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <CommonBuffers.hpp>

namespace Constants
{
	constexpr const char* appName  = "TerraTest";
}

class CommonBufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void CommonBufferTest::SetUpTestSuite()
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

void CommonBufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(CommonBufferTest, SharedBufferTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	SharedBuffer sharedBuffer{ logicalDevice, &memoryManager, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, {} };

	{
		VkDeviceSize offset = sharedBuffer.AllocateMemory(20_KB);
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 0u) << "Offset isn't 0.";
		EXPECT_EQ(size, 20_KB) << "Size isn't 20KB.";
	}

	// Need to do a GPU copy inbetween.
	// sharedBuffer.CopyOldBuffer(copyBuffer);

	{
		VkDeviceSize offset = sharedBuffer.AllocateMemory(30_KB);
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 20_KB) << "Offset isn't 20KB.";
		EXPECT_EQ(size, 20_KB + 30_KB) << "Size isn't 50KB.";
	}

	{
		VkDeviceSize offset = sharedBuffer.AllocateMemory(50_KB);
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 50_KB) << "Offset isn't 50KB.";
		EXPECT_EQ(size, 100_KB) << "Size isn't 100KB.";
	}

	sharedBuffer.RelinquishMemory(20_KB, 30_KB);

	{
		VkDeviceSize offset = sharedBuffer.AllocateMemory(20_KB);
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 20_KB) << "Offset isn't 20KB.";
		EXPECT_EQ(size, 100_KB) << "Size isn't 100KB.";
	}

	{
		VkDeviceSize offset = sharedBuffer.AllocateMemory(10_KB);
		VkDeviceSize size   = sharedBuffer.Size();

		EXPECT_EQ(offset, 40_KB) << "Offset isn't 40KB.";
		EXPECT_EQ(size, 100_KB) << "Size isn't 100KB.";
	}
}
