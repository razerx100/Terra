#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkTextureView.hpp>
#include <PipelineLayout.hpp>

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
	inline static std::unique_ptr<VkInstanceManager> s_instanceManager;
	inline static std::unique_ptr<VkDeviceManager>   s_deviceManager;
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

	s_deviceManager->SetDeviceFeatures(coreVersion)
		.SetPhysicalDeviceAutomatic(vkInstance)
		.CreateLogicalDevice();
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

		Buffer testStorage{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		testStorage.Create(2_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

		Buffer testUniform{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		testUniform.Create(2_KB, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, {});

		Buffer testTexel{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		testTexel.Create(2_KB, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, {});

		Buffer testStorageM{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		testStorageM.Create(4_KB, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

		VkDescriptorBuffer descBuffer{ logicalDevice, &memoryManager };
		descBuffer.SetDescriptorBufferInfo(physicalDevice);
		descBuffer.AddLayout(0u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddLayout(1u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddLayout(
			2u, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descBuffer.AddLayout(3u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddLayout(4u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.CreateBuffer();

		descBuffer.AddStorageBufferDescriptor(testStorage, 0u);
		descBuffer.AddUniformBufferDescriptor(testUniform, 1u);
		descBuffer.AddStorageTexelBufferDescriptor(testTexel, 2u, VK_FORMAT_R8G8B8A8_UINT);
		descBuffer.AddStorageBufferDescriptor(
			testStorageM, 3u, 0u, static_cast<VkDeviceSize>(2_KB)
		);
		descBuffer.AddStorageBufferDescriptor(
			testStorageM, 4u,
			static_cast<VkDeviceAddress>(2_KB), static_cast<VkDeviceSize>(2_KB)
		);
	}

	{
		MemoryManager memoryManager{ physicalDevice, logicalDevice, 200_MB, 200_KB };

		VkSamplerCreateInfoBuilder samplerCreateInfo{};

		VKSampler sampler{ logicalDevice };
		sampler.CreateSampler(samplerCreateInfo.GetPtr());

		VkDescriptorBuffer descBuffer{ logicalDevice, &memoryManager };
		descBuffer.SetDescriptorBufferInfo(physicalDevice);
		descBuffer.AddLayout(
			0u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descBuffer.AddLayout(1u, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddLayout(2u, VK_DESCRIPTOR_TYPE_SAMPLER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddLayout(3u, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.CreateBuffer();

		VkTextureView textureView{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		textureView.CreateView2D(
			200u, 200u, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D, {}
		);
		VkTextureView storageView{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		storageView.CreateView3D(
			200u, 200u, 50u, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_USAGE_STORAGE_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_3D, {}
		);

		descBuffer.AddCombinedImageDescriptor(
			textureView.GetView(), sampler.Get(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0u
		);
		descBuffer.AddSampledImageDescriptor(
			textureView.GetView(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1u
		);
		descBuffer.AddSamplerDescriptor(sampler.Get(), 2u);
		descBuffer.AddStorageImageDescriptor(
			storageView.GetView(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 3u
		);

		// PipelineLayout
		PipelineLayout pipelineLayout{ logicalDevice };
		pipelineLayout.Create(descBuffer);
	}
}
