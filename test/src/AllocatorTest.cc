#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkResources.hpp>

namespace Constants
{
	constexpr const char* appName  = "TerraTest";
}

class AllocatorTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void AllocatorTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
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

void AllocatorTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(AllocatorTest, VkAllocatorTest)
{
	VkDevice logicalDevice = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 2_MB, 200_KB };

	{
		Buffer buffer{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		buffer.Create(1_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

		EXPECT_NE(buffer.Get(), VK_NULL_HANDLE) << "Buffer wasn't initialised";
		EXPECT_EQ(buffer.CPUHandle(), nullptr) << "CPU Pointer isn't null.";
		EXPECT_EQ(buffer.BufferSize(), 1_KB) << "BufferSize doesn't match.";
	}
}
