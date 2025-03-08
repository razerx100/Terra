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
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
};

void BufferTest::SetUpTestSuite()
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

	EXPECT_EQ(testTexel.BufferSize(), 2_KB) << "Buffer size isn't 2_KB.";
	EXPECT_NE(testTexel.Get(), VK_NULL_HANDLE) << "Buffer wasn't created.";

	testTexel.Create(4_KB, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, {});

	EXPECT_EQ(testTexel.BufferSize(), 4_KB) << "Buffer size isn't 2_KB.";
	EXPECT_NE(testTexel.Get(), VK_NULL_HANDLE) << "Buffer wasn't created.";
}

TEST_F(BufferTest, VkSamplerTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	VkSamplerCreateInfoBuilder::SetMaxAnisotropy(physicalDevice);
	VkSamplerCreateInfoBuilder createInfoBuilder{};

	VKSampler sampler{ logicalDevice };
	sampler.Create(createInfoBuilder);

	createInfoBuilder
		.Anisotropy(2.f)
		.BorderColour(VK_BORDER_COLOR_INT_OPAQUE_BLACK)
		.MagFilter(VK_FILTER_NEAREST);

	VKSampler sampler1{ logicalDevice };
	sampler1.Create(createInfoBuilder);
}

TEST_F(BufferTest, TextureTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	Texture testTexture{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testTexture.Create2D(1280u, 720u, 1u, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, {});

	EXPECT_EQ(testTexture.GetExtent().width, 1280u) << "Texture width isn't 1280.";
	EXPECT_EQ(testTexture.GetExtent().height, 720u) << "Texture width isn't 720.";
	EXPECT_EQ(testTexture.GetBufferSize(), 3'686'400lu) << "Texture size isn't 3'686'400.";
	EXPECT_NE(testTexture.Get(), VK_NULL_HANDLE) << "Texture wasn't created.";

	testTexture.Create2D(1920u, 1080u, 1u, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, {});

	EXPECT_EQ(testTexture.GetExtent().width, 1920u) << "Texture width isn't 1920.";
	EXPECT_EQ(testTexture.GetExtent().height, 1080u) << "Texture width isn't 1080.";
	EXPECT_EQ(testTexture.GetBufferSize(), 8'294'400lu) << "Texture size isn't 8'294'400.";
	EXPECT_NE(testTexture.Get(), VK_NULL_HANDLE) << "Texture wasn't created.";
}

TEST_F(BufferTest, TextureViewTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

	VkTextureView testTextureView{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
	testTextureView.CreateView2D(
		1280u, 720u, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_2D, {}
	);

	VkTextureView testMoveTextureView = std::move(testTextureView);
}
