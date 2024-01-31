#include <gtest/gtest.h>
#include <memory>

#include <VKInstanceManager.hpp>
#include <VkDeviceManager.hpp>
#include <VkResources.hpp>

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
		// Set Layout
		VkDescriptorSetLayoutBinding layoutBinding{
			.binding         = 0u,
			.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1u,
			.stageFlags      = VK_SHADER_STAGE_VERTEX_BIT
		};

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{
			.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.flags        = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT,
			.bindingCount = 1u,
			.pBindings    = &layoutBinding
		};

		VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;
		vkCreateDescriptorSetLayout(logicalDevice, &layoutCreateInfo, nullptr, &descLayout);

		// Descriptor Buffer Properties
		VkPhysicalDeviceDescriptorBufferPropertiesEXT descBufferProperties{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT
		};

		VkPhysicalDeviceProperties2 physicalDeviceProp2{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
			.pNext = &descBufferProperties
		};

		vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProp2);

		// Memory requirement for Set Layout
		VkDeviceSize layoutSizeInBytes = 0u;
		VkDeviceSize layoutOffset      = 0u;

		DescBuffer::vkGetDescriptorSetLayoutSizeEXT(logicalDevice, descLayout, &layoutSizeInBytes);

		DescBuffer::vkGetDescriptorSetLayoutBindingOffsetEXT(
			logicalDevice, descLayout, 0u, &layoutOffset
		);

		MemoryManager memoryManager{ physicalDevice, logicalDevice, 20_MB, 200_KB };

		Buffer testBuffer{ logicalDevice, memoryManager, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT };
		testBuffer.Create(
			logicalDevice, 2_KB,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			{}
		);

		VkDeviceAddress testBufferAddress = testBuffer.GpuPhysicalAddress();

		VkDescriptorAddressInfoEXT storageBufferInfo{
			.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT,
			.address = testBufferAddress,
			.range   = 2_KB
		};

		VkDescriptorDataEXT descData{ .pStorageBuffer = &storageBufferInfo };
		VkDescriptorGetInfoEXT getInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.data  = descData
		};

		Buffer descBuffer{ logicalDevice, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };
		descBuffer.Create(
			logicalDevice, layoutSizeInBytes, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT, {}
		);

		DescBuffer::vkGetDescriptorEXT(logicalDevice, &getInfo, descBufferProperties.storageBufferDescriptorSize, descBuffer.CPUHandle());

		VkDescriptorBufferBindingInfoEXT bindingInfo{};

		vkDestroyDescriptorSetLayout(logicalDevice, descLayout, nullptr);
	}
}
