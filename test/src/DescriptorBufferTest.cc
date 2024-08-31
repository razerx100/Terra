#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkTextureView.hpp>
#include <PipelineLayout.hpp>

namespace Constants
{
	constexpr const char* appName    = "TerraTest";
	constexpr std::uint32_t setCount = 2u;
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
	s_instanceManager->DebugLayers().AddDebugCallback(DebugCallbackType::StandardError);
	s_instanceManager->CreateInstance(coreVersion);
	VkInstance vkInstance = s_instanceManager->GetVKInstance();

	s_deviceManager = std::make_unique<VkDeviceManager>();

	{
		VkDeviceExtensionManager& extensionManager = s_deviceManager->ExtensionManager();
		extensionManager.AddExtensions(MemoryManager::GetRequiredExtensions());
		extensionManager.AddExtensions(VkDescriptorBuffer::GetRequiredExtensions());
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

		VkDescriptorBuffer descBuffer{ logicalDevice, &memoryManager, Constants::setCount };
		descBuffer.SetDescriptorBufferInfo(physicalDevice);
		descBuffer.AddBinding(0u, 0u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddBinding(1u, 0u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddBinding(
			2u, 0u, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descBuffer.AddBinding(3u, 0u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddBinding(4u, 0u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);

		descBuffer.AddBinding(0u, 1u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT);
		descBuffer.AddBinding(1u, 1u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT);

		descBuffer.CreateBuffer();

		descBuffer.SetStorageBufferDescriptor(testStorage, 0u, 0u, 0u);
		descBuffer.SetUniformBufferDescriptor(testUniform, 1u, 0u, 0u);
		descBuffer.SetStorageTexelBufferDescriptor(testTexel, 2u, 0u, 0u, VK_FORMAT_R8G8B8A8_UINT);
		descBuffer.SetStorageBufferDescriptor(
			testStorageM, 3u, 0u, 0u, 0u, static_cast<VkDeviceSize>(2_KB)
		);
		descBuffer.SetStorageBufferDescriptor(
			testStorageM, 4u, 0u, 0u,
			static_cast<VkDeviceAddress>(2_KB), static_cast<VkDeviceSize>(2_KB)
		);

		descBuffer.SetStorageBufferDescriptor(testStorage, 0u, 1u, 0u);
		descBuffer.SetUniformBufferDescriptor(testUniform, 1u, 1u, 0u);
	}

	{
		MemoryManager memoryManager{ physicalDevice, logicalDevice, 200_MB, 200_KB };

		VkSamplerCreateInfoBuilder samplerCreateInfo{};

		VKSampler sampler{ logicalDevice };
		sampler.Create(samplerCreateInfo);

		VkDescriptorBuffer descBuffer{ logicalDevice, &memoryManager, Constants::setCount };
		descBuffer.SetDescriptorBufferInfo(physicalDevice);
		descBuffer.AddBinding(
			0u, 0u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
		);
		descBuffer.AddBinding(1u, 0u, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddBinding(2u, 0u, VK_DESCRIPTOR_TYPE_SAMPLER, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);
		descBuffer.AddBinding(3u, 0u, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, VK_SHADER_STAGE_FRAGMENT_BIT);

		descBuffer.AddBinding(0u, 1u, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1u, VK_SHADER_STAGE_VERTEX_BIT);

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

		descBuffer.SetCombinedImageDescriptor(
			textureView, sampler, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0u, 0u, 0u
		);
		descBuffer.SetSampledImageDescriptor(
			textureView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1u, 0u, 0u
		);
		descBuffer.SetSamplerDescriptor(sampler, 2u, 0u, 0u);
		descBuffer.SetStorageImageDescriptor(
			storageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 3u, 0u, 0u
		);

		descBuffer.SetStorageImageDescriptor(
			storageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0u, 1u, 0u
		);

		// PipelineLayout
		PipelineLayout pipelineLayout{ logicalDevice };
		pipelineLayout.Create(descBuffer.GetLayouts());
	}
}

TEST_F(DescriptorBufferTest, DescriptorBufferCopyTest)
{
	VkDevice logicalDevice          = s_deviceManager->GetLogicalDevice();
	VkPhysicalDevice physicalDevice = s_deviceManager->GetPhysicalDevice();

	using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

	{
		MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

		VkTextureView textureView{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		textureView.CreateView2D(
			200u, 200u, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D, {}
		);

		VkTextureView textureView1{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		textureView1.CreateView2D(
			200u, 200u, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D, {}
		);

		VkTextureView textureView2{ logicalDevice, &memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		textureView2.CreateView2D(
			200u, 200u, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_VIEW_TYPE_2D, {}
		);

		VkDescriptorBuffer descBuffer{ logicalDevice, &memoryManager, Constants::setCount };
		{
			descBuffer.SetDescriptorBufferInfo(physicalDevice);
			descBuffer.AddBinding(
				0u, 0u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 65u, VK_SHADER_STAGE_VERTEX_BIT
			);
			descBuffer.AddBinding(
				0u, 1u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 65u, VK_SHADER_STAGE_FRAGMENT_BIT
			);

			descBuffer.CreateBuffer();

			descBuffer.AddBinding(
				0u, 1u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 65535u, VK_SHADER_STAGE_FRAGMENT_BIT
			);

			descBuffer.AddBinding(
				1u, 1u, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 255u, VK_SHADER_STAGE_FRAGMENT_BIT
			);

			descBuffer.RecreateBuffer();
		}

		{
			VkDescriptorBuffer descBuffer1{ logicalDevice, &memoryManager, Constants::setCount };
			descBuffer1.SetDescriptorBufferInfo(physicalDevice);

			descBuffer1.AddBinding(
				0u, 0u, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2u, VK_SHADER_STAGE_FRAGMENT_BIT
			);

			descBuffer1.AddBinding(
				0u, 1u, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1u, VK_SHADER_STAGE_FRAGMENT_BIT
			);

			descBuffer1.CreateBuffer();

			VkSamplerCreateInfoBuilder samplerCreateInfo{};

			VKSampler sampler{ logicalDevice };
			sampler.Create(samplerCreateInfo);

			{
				void const* desc = descBuffer1.GetCombinedImageDescriptor(
					textureView, sampler, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0u, 0u, 0u
				);

				descBuffer.SetCombinedImageDescriptor(desc, 0u, 0u, 0u);
			}

			{
				void const* desc = descBuffer1.GetCombinedImageDescriptor(
					textureView1, sampler, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0u, 0u, 0u
				);

				descBuffer.SetCombinedImageDescriptor(desc, 0u, 0u, 1u);
			}

			{
				void const* desc = descBuffer1.GetSampledImageDescriptor(
					textureView2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 0u, 1u, 0u
				);

				descBuffer.SetSampledImageDescriptor(desc, 0u, 1u, 0u);
			}
		}
	}
}
