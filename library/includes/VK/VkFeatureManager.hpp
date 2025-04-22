#ifndef VK_FEATURE_MANAGER_HPP_
#define VK_FEATURE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <VkExtensionManager.hpp>
#include <VkStructChain.hpp>
#include <vector>
#include <cstring>

namespace Terra
{
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
	{
		return m_deviceFeatures2.GetPtr();
	}

private:
	void SetBaseCoreFeatures() noexcept;
	void Set1_1CoreFeatures() noexcept;
	void Set1_2CoreFeatures() noexcept;
	void Set1_3CoreFeatures() noexcept;

	void SetVkExtMeshShaderFeatures() noexcept;
	void SetVkExtDescriptorBufferFeatures() noexcept;

private:
	using MembersType = std::vector<size_t>;

private:
	VkStructChain<VkPhysicalDeviceFeatures2> m_deviceFeatures2;
	MembersType                              m_feature1Members;
	std::vector<MembersType>                 m_chainStructMembers;

private:
	template<typename T>
	[[nodiscard]]
	static bool CheckMembers(const std::vector<size_t>& members, const T& object) noexcept
	{
		bool allFeaturesSupported = true;

		for (size_t memberOffset : members)
		{
			// All of these members we are checking should be VkBool32.
			VkBool32 member{ 0u };

			auto objectStart = reinterpret_cast<std::uint8_t const*>(&object);

			memcpy(&member, objectStart + memberOffset, sizeof(VkBool32));

			allFeaturesSupported &= (member == VK_TRUE);
		}

		return allFeaturesSupported;
	}

	template<typename T>
	static void AddMember(std::vector<size_t>& members, size_t memberOffset, T& object) noexcept
	{
		members.emplace_back(memberOffset);

		// The member at the offset should be a VkBool32. The next statement looks bad, but should
		// be fine and not be UB as long as the member at the offset is a VkBool32.
		VkBool32& member = *reinterpret_cast<VkBool32*>(
			reinterpret_cast<std::uint8_t*>(&object) + memberOffset
		);

		member = VK_TRUE;
	}

	[[nodiscard]]
	static bool CheckFeatureSupportRecursive(
		void* structPtr, const std::vector<MembersType>& chainMembers, size_t memberIndex = 0u
	) noexcept;

public:
	VkFeatureManager(const VkFeatureManager& other) noexcept
		: m_deviceFeatures2{ other.m_deviceFeatures2 }, m_feature1Members{ other.m_feature1Members },
		m_chainStructMembers{ other.m_chainStructMembers }
	{}
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
		m_chainStructMembers{ std::move(other.m_chainStructMembers) }
	{}
	VkFeatureManager& operator=(VkFeatureManager&& other) noexcept
	{
		m_deviceFeatures2    = std::move(other.m_deviceFeatures2);
		m_feature1Members    = std::move(other.m_feature1Members);
		m_chainStructMembers = std::move(other.m_chainStructMembers);

		return *this;
	}
};
}
#endif
