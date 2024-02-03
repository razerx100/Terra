#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkDescriptorBuffer.hpp>

namespace Constants
{
	constexpr const char* appName  = "TerraTest";
}

class DescriptorBufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager>   s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>     s_deviceManager;
};

void DescriptorBufferTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::standardError);
	s_instanceManager->CreateInstance(coreVersion);
	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	{
		VkDeviceExtensionManager& extensionManager = s_deviceManager->ExtensionManager();
		extensionManager.AddExtension(DeviceExtension::VkExtDescriptorBuffer);
	}

	s_deviceManager->FindPhysicalDevice(vkInstance, VK_NULL_HANDLE).CreateLogicalDevice(coreVersion);
}

void DescriptorBufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(DescriptorBufferTest, DescriptorBufferTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	{
		MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

		Buffer testBuffer{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		testBuffer.Create(2_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

		Buffer testBuffer1{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		testBuffer1.Create(2_KB, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, {});

		VkDescriptorBuffer descBuffer{ logicalDevice, &memoryManager };
		descBuffer.GetDescriptorBufferInfo(physicalDevice);
		descBuffer.AddLayout(0u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddLayout(1u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.CreateBuffer();

		descBuffer.AddStorageBufferDescriptor(testBuffer, 0u, 0u);
		descBuffer.AddUniformBufferDescriptor(testBuffer1, 1u, 0u);
	}
}
