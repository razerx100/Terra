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
	VkPhysicalDeviceFeatures baseFeatures{};

	AddMember(m_feature1Members, &VkPhysicalDeviceFeatures::multiDrawIndirect, baseFeatures);
	AddMember(m_feature1Members, &VkPhysicalDeviceFeatures::drawIndirectFirstInstance, baseFeatures);
	AddMember(m_feature1Members, &VkPhysicalDeviceFeatures::samplerAnisotropy, baseFeatures);

	m_deviceFeatures2.Get().features = baseFeatures;
}

void VkFeatureManager::Set1_1CoreFeatures() noexcept
{
	auto pV1_1Features = std::make_shared<VkPhysicalDeviceVulkan11Features>();
	VkPhysicalDeviceVulkan11Features& v1_1Features = *pV1_1Features;

	v1_1Features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

	{
		MembersType core1_1Type{};

		AddMember(
			core1_1Type, &VkPhysicalDeviceVulkan11Features::shaderDrawParameters, v1_1Features
		);

		m_chainStructMembers.emplace_back(std::move(core1_1Type));
	}

	m_deviceFeatures2.AddToChain(std::move(pV1_1Features));
}

void VkFeatureManager::Set1_2CoreFeatures() noexcept
{
	auto pV1_2Features = std::make_shared<VkPhysicalDeviceVulkan12Features>();
	VkPhysicalDeviceVulkan12Features& v1_2Features = *pV1_2Features;

	v1_2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

	{
		MembersType core1_2Type{};

		AddMember(
			core1_2Type, &VkPhysicalDeviceVulkan12Features::drawIndirectCount, v1_2Features
		);
		AddMember(
			core1_2Type, &VkPhysicalDeviceVulkan12Features::descriptorIndexing, v1_2Features
		);
		AddMember(
			core1_2Type, &VkPhysicalDeviceVulkan12Features::shaderSampledImageArrayNonUniformIndexing,
			v1_2Features
		);
		AddMember(
			core1_2Type,
			&VkPhysicalDeviceVulkan12Features::descriptorBindingUniformBufferUpdateAfterBind,
			v1_2Features
		);
		AddMember(
			core1_2Type,
			&VkPhysicalDeviceVulkan12Features::descriptorBindingSampledImageUpdateAfterBind,
			v1_2Features
		);
		AddMember(
			core1_2Type,
			&VkPhysicalDeviceVulkan12Features::descriptorBindingStorageBufferUpdateAfterBind,
			v1_2Features
		);
		AddMember(
			core1_2Type, &VkPhysicalDeviceVulkan12Features::descriptorBindingPartiallyBound,
			v1_2Features
		);
		AddMember(
			core1_2Type, &VkPhysicalDeviceVulkan12Features::runtimeDescriptorArray, v1_2Features
		);
		AddMember(
			core1_2Type, &VkPhysicalDeviceVulkan12Features::bufferDeviceAddress, v1_2Features
		);
		AddMember(
			core1_2Type, &VkPhysicalDeviceVulkan12Features::timelineSemaphore, v1_2Features
		);

		m_chainStructMembers.emplace_back(std::move(core1_2Type));
	}

	m_deviceFeatures2.AddToChain(std::move(pV1_2Features));
}

void VkFeatureManager::Set1_3CoreFeatures() noexcept
{
	auto pV1_3Features = std::make_shared<VkPhysicalDeviceVulkan13Features>();
	VkPhysicalDeviceVulkan13Features& v1_3Features = *pV1_3Features;

	v1_3Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

	{
		MembersType core1_3Type{};

		AddMember(
			core1_3Type, &VkPhysicalDeviceVulkan13Features::synchronization2, v1_3Features
		);

		m_chainStructMembers.emplace_back(std::move(core1_3Type));
	}

	m_deviceFeatures2.AddToChain(std::move(pV1_3Features));
}

void VkFeatureManager::SetVkExtMeshShaderFeatures() noexcept
{
	auto pVkExtMeshShaderFeatures = std::make_shared<VkPhysicalDeviceMeshShaderFeaturesEXT>();
	VkPhysicalDeviceMeshShaderFeaturesEXT& vkExtMeshShaderFeatures = *pVkExtMeshShaderFeatures;

	vkExtMeshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;

	{
		MembersType meshShaderType{};

		AddMember(
			meshShaderType, &VkPhysicalDeviceMeshShaderFeaturesEXT::meshShader, vkExtMeshShaderFeatures
		);
		AddMember(
			meshShaderType, &VkPhysicalDeviceMeshShaderFeaturesEXT::taskShader, vkExtMeshShaderFeatures
		);

		m_chainStructMembers.emplace_back(std::move(meshShaderType));
	}

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

	{
		MembersType descriptorBufferType{};

		AddMember(
			descriptorBufferType, &VkPhysicalDeviceDescriptorBufferFeaturesEXT::descriptorBuffer,
			vkExtDescriptorBufferFeatures
		);

		m_chainStructMembers.emplace_back(std::move(descriptorBufferType));
	}

	m_deviceFeatures2.AddToChain(std::move(pVkExtDescriptorBufferFeatures));
}

bool VkFeatureManager::CheckFeatureSupport(VkPhysicalDevice device) const noexcept
{
	// This needs to be a new object, since it will be overwritten by the query.
	VkPhysicalDeviceFeatures2 features2Check = m_deviceFeatures2.Get();
	vkGetPhysicalDeviceFeatures2(device, &features2Check);

	const bool feature1Check = CheckMembers(m_feature1Members, features2Check.features);

	return feature1Check && CheckFeatureSupportRecursive(features2Check.pNext, m_chainStructMembers);
}

bool VkFeatureManager::CheckFeatureSupportRecursive(
	void* structPtr, const std::vector<MembersType>& chainMembers, size_t memberIndex /* = 0u */
) noexcept {
	bool result = true;

	if (structPtr)
	{
		struct PNextStruct
		{
			VkStructureType sType;
			void*           pNext;
		};

		auto pNextStruct = reinterpret_cast<PNextStruct*>(structPtr);
		switch (pNextStruct->sType)
		{
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT:
		{
			auto structObj = reinterpret_cast<VkPhysicalDeviceDescriptorBufferFeaturesEXT*>(structPtr);
			result        &= CheckMembers(chainMembers.at(memberIndex), *structObj);
			break;
		}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT:
		{
			auto structObj = reinterpret_cast<VkPhysicalDeviceMeshShaderFeaturesEXT*>(structPtr);
			result        &= CheckMembers(chainMembers.at(memberIndex), *structObj);
			break;
		}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES:
		{
			auto structObj = reinterpret_cast<VkPhysicalDeviceVulkan13Features*>(structPtr);
			result        &= CheckMembers(chainMembers.at(memberIndex), *structObj);
			break;

		}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES:
		{
			auto structObj = reinterpret_cast<VkPhysicalDeviceVulkan12Features*>(structPtr);
			result        &= CheckMembers(chainMembers.at(memberIndex), *structObj);
			break;

		}
		case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES:
		{
			auto structObj = reinterpret_cast<VkPhysicalDeviceVulkan11Features*>(structPtr);
			result        &= CheckMembers(chainMembers.at(memberIndex), *structObj);
			break;
		}
		}

		return result && CheckFeatureSupportRecursive(pNextStruct->pNext, chainMembers, ++memberIndex);
	}

	return result;
}

void VkFeatureManager::ClearFeatureChecks() noexcept
{
	m_feature1Members    = MembersType{};
	m_chainStructMembers = std::vector<MembersType>{};
}
