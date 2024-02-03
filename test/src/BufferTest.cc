#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkResources.hpp>
#include <VkTextureView.hpp>

namespace Constants
{
	constexpr const char* appName  = "TerraTest";
}

class BufferTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite();
	static void TearDownTestSuite();

protected:
	inline static std::unique_ptr<VkInstanceManager>   s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>     s_deviceManager;
};

void BufferTest::SetUpTestSuite()
{
	const CoreVersion coreVersion = CoreVersion::V1_3;

	s_instanceManager = std::make_unique<VkInstanceManager>(Constants::appName);
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::standardError);
	s_instanceManager->CreateInstance(coreVersion);
	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	s_deviceManager->FindPhysicalDevice(vkInstance, VK_NULL_HANDLE).CreateLogicalDevice(coreVersion);
}

void BufferTest::TearDownTestSuite()
{
	s_deviceManager.reset();
	s_instanceManager.reset();
}

TEST_F(BufferTest, VkBufferTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	Buffer testStorage{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testStorage.Create(2_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

	Buffer testIndirect{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testIndirect.Create(2_KB, VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, {});

	Buffer testTexel{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testTexel.Create(2_KB, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, {});
}

TEST_F(BufferTest, VkSamplerTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	VkSamplerCreateInfoBuilder::SetMaxAnisotropy(physicalDevice);
	VkSamplerCreateInfoBuilder createInfoBuilder{};

	VKSampler sampler{ logicalDevice };
	sampler.CreateSampler(createInfoBuilder.GetPtr());

	createInfoBuilder
		.Anisotropy(2.f)
		.BorderColour(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
		.MagFilter(VK_FILTER_NEAREST);

	VKSampler sampler1{ logicalDevice };
	sampler1.CreateSampler(createInfoBuilder.Get());
}
