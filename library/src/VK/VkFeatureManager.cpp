#include <VkFeatureManager.hpp>

template<class T>
static constexpr size_t EnumToNumber(T value)
{
	return static_cast<size_t>(value);
}

VkFeatureManager::VkFeatureManager() : m_deviceFeatures2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 }
{
	SetBaseCoreFeatures();
}

void VkFeatureManager::SetCoreFeatures(CoreVersion version) noexcept
{
	const size_t versionNumber = EnumToNumber(version);

	if (EnumToNumber(CoreVersion::V1_1) <= versionNumber)
		Set1_1CoreFeatures();

	if (EnumToNumber(CoreVersion::V1_2) <= versionNumber)
		Set1_2CoreFeatures();

	if (EnumToNumber(CoreVersion::V1_3) <= versionNumber)
		Set1_3CoreFeatures();
}

void VkFeatureManager::SetExtensionFeatures(DeviceExtension extension) noexcept
{
	if (DeviceExtension::VkExtMeshShader == extension)
		SetVkExtMeshShaderFeatures();

	if (DeviceExtension::VkExtDescriptorBuffer == extension)
		SetVkExtDescriptorBufferFeatures();
}

void VkFeatureManager::SetBaseCoreFeatures() noexcept
{
	VkPhysicalDeviceFeatures baseFeatures{
		.multiDrawIndirect         = VK_TRUE,
		.drawIndirectFirstInstance = VK_TRUE,
		.samplerAnisotropy         = VK_TRUE
	};

	m_deviceFeatures2.Get().features = baseFeatures;
}

void VkFeatureManager::Set1_1CoreFeatures() noexcept
{
	auto pV1_1Features = std::make_shared<VkPhysicalDeviceVulkan11Features>();
	VkPhysicalDeviceVulkan11Features& v1_1Features = *pV1_1Features;

	v1_1Features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
	v1_1Features.shaderDrawParameters = VK_TRUE;

	m_deviceFeatures2.AddToChain(std::move(pV1_1Features));
}

void VkFeatureManager::Set1_2CoreFeatures() noexcept
{
	auto pV1_2Features = std::make_shared<VkPhysicalDeviceVulkan12Features>();
	VkPhysicalDeviceVulkan12Features& v1_2Features = *pV1_2Features;

	v1_2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	v1_2Features.drawIndirectCount                             = VK_TRUE;
	v1_2Features.descriptorIndexing                            = VK_TRUE;
	v1_2Features.shaderSampledImageArrayNonUniformIndexing     = VK_TRUE;
	v1_2Features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
	v1_2Features.descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE;
	v1_2Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
	v1_2Features.runtimeDescriptorArray                        = VK_TRUE;
	v1_2Features.bufferDeviceAddress                           = VK_TRUE;

	m_deviceFeatures2.AddToChain(std::move(pV1_2Features));
}

void VkFeatureManager::Set1_3CoreFeatures() noexcept
{
	auto pV1_3Features = std::make_shared<VkPhysicalDeviceVulkan13Features>();
	VkPhysicalDeviceVulkan13Features& v1_3Features = *pV1_3Features;

	v1_3Features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	v1_3Features.synchronization2 = VK_TRUE;

	m_deviceFeatures2.AddToChain(std::move(pV1_3Features));
}

void VkFeatureManager::SetVkExtMeshShaderFeatures() noexcept
{
	auto pVkExtMeshShaderFeatures = std::make_shared<VkPhysicalDeviceMeshShaderFeaturesEXT>();
	VkPhysicalDeviceMeshShaderFeaturesEXT& vkExtMeshShaderFeatures = *pVkExtMeshShaderFeatures;

	vkExtMeshShaderFeatures.sType      = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	vkExtMeshShaderFeatures.meshShader = VK_TRUE;
	vkExtMeshShaderFeatures.taskShader = VK_TRUE;

	m_deviceFeatures2.AddToChain(std::move(pVkExtMeshShaderFeatures));
}

void VkFeatureManager::SetVkExtDescriptorBufferFeatures() noexcept
{
	auto pVkExtDescriptorBufferFeatures
		= std::make_shared<VkPhysicalDeviceDescriptorBufferFeaturesEXT>();
	VkPhysicalDeviceDescriptorBufferFeaturesEXT& vkExtDescriptorBufferFeatures
		= *pVkExtDescriptorBufferFeatures;

	vkExtDescriptorBufferFeatures.sType
		= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT;
	vkExtDescriptorBufferFeatures.descriptorBuffer = VK_TRUE;

	m_deviceFeatures2.AddToChain(std::move(pVkExtDescriptorBufferFeatures));
}
