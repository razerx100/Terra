#ifndef VK_FEATURE_MANAGER_HPP_
#define VK_FEATURE_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <VkExtensionManager.hpp>
#include <VkStructChain.hpp>

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
	VkStructChain<VkPhysicalDeviceFeatures2> m_deviceFeatures2;
};
#endif
