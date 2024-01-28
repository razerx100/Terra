#include <VkExtensionManager.hpp>
#include <Exception.hpp>
#include <array>

// Extension Manager
static std::array extensionNameMap
{
	"VK_EXT_mesh_shader",
	"VK_KHR_swapchain",
	"VK_EXT_memory_budget"
};

void VkDeviceExtensionManager::AddExtension(DeviceExtension extension) noexcept
{
	const auto extensionIndex = static_cast<size_t>(extension);
	m_extensions.set(extensionIndex);
	m_extensionNames.emplace_back(extensionNameMap.at(extensionIndex));
}

void VkDeviceExtensionManager::PopulateExtensionFunctions(VkDevice device) const noexcept
{
	if (m_extensions.test(static_cast<size_t>(DeviceExtension::VkExtMeshShader)))
		PopulateVkExtMeshShader(device);
}

const std::vector<const char*>& VkDeviceExtensionManager::GetExtensionNames() const noexcept
{
	return m_extensionNames;
}

bool VkDeviceExtensionManager::IsExtensionActive(DeviceExtension extension) const noexcept
{
	return m_extensions.test(static_cast<size_t>(extension));
}

void VkDeviceExtensionManager::PopulateVkExtMeshShader(VkDevice device) noexcept
{
	using namespace VkDeviceExtension;

	PopulateFunctionPointer(device, "vkCmdDrawMeshTasksEXT", VkExtMeshShader::s_vkCmdDrawMeshTasksEXT);
	PopulateFunctionPointer(
		device, "vkCmdDrawMeshTasksIndirectCountEXT", VkExtMeshShader::s_vkCmdDrawMeshTasksIndirectCountEXT
	);
	PopulateFunctionPointer(
		device, "vkCmdDrawMeshTasksIndirectEXT", VkExtMeshShader::s_vkCmdDrawMeshTasksIndirectEXT
	);
}
