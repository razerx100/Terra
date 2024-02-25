#ifndef VK_FEATURE_MANAGER_HPP_
#define VK_FEATURE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <VkExtensionManager.hpp>
#include <VkStructChain.hpp>
#include <vector>

enum class CoreVersion
{
	V1_0,
	V1_1,
	V1_2,
	V1_3
};

class VkFeatureManager
{
public:
	VkFeatureManager();

	void SetCoreFeatures(CoreVersion version) noexcept;
	void SetExtensionFeatures(DeviceExtension extension) noexcept;

	void ClearFeatureChecks() noexcept;

	[[nodiscard]]
	bool CheckFeatureSupport(VkPhysicalDevice device) const noexcept;
	[[nodiscard]]
	VkPhysicalDeviceFeatures2 const* GetDeviceFeatures2() const noexcept
	{ return m_deviceFeatures2.GetPtr(); }

private:
	void SetBaseCoreFeatures() noexcept;
	void Set1_1CoreFeatures() noexcept;
	void Set1_2CoreFeatures() noexcept;
	void Set1_3CoreFeatures() noexcept;

	void SetVkExtMeshShaderFeatures() noexcept;
	void SetVkExtDescriptorBufferFeatures() noexcept;

private:
	using MembersType = std::vector<void*>;

private:
	VkStructChain<VkPhysicalDeviceFeatures2> m_deviceFeatures2;
	MembersType                              m_feature1Members;
	std::vector<MembersType>                 m_chainStructMembers;

private:
	template<typename T>
	union MemberUnion
	{
		VkBool32 T::* pMember;
		void*         pVoid;
	};

	template<typename T>
	[[nodiscard]]
	static bool CheckMembers(const std::vector<void*>& members, T& object) noexcept
	{
		bool allFeaturesSupported = true;
		for (void* member : members)
		{
			MemberUnion<T> memberUnion{ .pVoid = member };
			auto actualMember     = memberUnion.pMember;
			allFeaturesSupported &= static_cast<bool>(std::invoke(actualMember, object));
		}

		return allFeaturesSupported;
	}

	template<typename T>
	static void AddMember(std::vector<void*>& members, VkBool32 T::* memberPtr, T& object) noexcept
	{
		MemberUnion<T> member{ .pMember = memberPtr };
		members.emplace_back(member.pVoid);

		object.*memberPtr = VK_TRUE;
	}

	[[nodiscard]]
	static bool CheckFeatureSupportRecursive(
		void* structPtr, const std::vector<MembersType>& chainMembers, size_t memberIndex = 0u
	) noexcept;

public:
	VkFeatureManager(const VkFeatureManager& other) noexcept
		: m_deviceFeatures2{ other.m_deviceFeatures2 }, m_feature1Members{ other.m_feature1Members },
		m_chainStructMembers{ other.m_chainStructMembers } {}
	VkFeatureManager& operator=(const VkFeatureManager& other) noexcept
	{
		m_deviceFeatures2    = other.m_deviceFeatures2;
		m_feature1Members    = other.m_feature1Members;
		m_chainStructMembers = other.m_chainStructMembers;

		return *this;
	}
	VkFeatureManager(VkFeatureManager&& other) noexcept
		: m_deviceFeatures2{ std::move(other.m_deviceFeatures2) },
		m_feature1Members{ std::move(other.m_feature1Members) },
		m_chainStructMembers{ std::move(other.m_chainStructMembers) } {}
	VkFeatureManager& operator=(VkFeatureManager&& other) noexcept
	{
		m_deviceFeatures2    = std::move(other.m_deviceFeatures2);
		m_feature1Members    = std::move(other.m_feature1Members);
		m_chainStructMembers = std::move(other.m_chainStructMembers);

		return *this;
	}
};
#endif
