#include <VkExtensionLoader.hpp>
#include <Exception.hpp>
#include <array>

// Extension Manager
static std::array extensionNameMap
{
	"VK_EXT_mesh_shader"
};

void VkDeviceExtensionManager::AddExtension(DeviceExtension extension) noexcept
{
	m_extentions.emplace_back(extension);
}

void VkDeviceExtensionManager::PopulateExtensionFunctions(VkDevice device) const noexcept
{
	for (const auto extension : m_extentions)
	{
		if (extension == DeviceExtension::VkExtMeshShader)
			PopulateVkExtMeshShader(device);
	}
}

std::vector<const char*> VkDeviceExtensionManager::GetExtensionNames() const noexcept
{
	std::vector<const char*> extensionNames{};
	extensionNames.reserve(std::size(m_extentions));

	for (const auto extension : m_extentions)
		extensionNames.emplace_back(extensionNameMap.at(static_cast<size_t>(extension)));

	return extensionNames;
}

void VkDeviceExtensionManager::PopulateVkExtMeshShader(VkDevice device) noexcept
{
	using namespace VkExtension;

	PopulateFunctionPointer(device, "vkCmdDrawMeshTasksEXT", VkExtMeshShader::s_vkCmdDrawMeshTasksEXT);
	PopulateFunctionPointer(
		device, "vkCmdDrawMeshTasksIndirectCountEXT", VkExtMeshShader::s_vkCmdDrawMeshTasksIndirectCountEXT
	);
	PopulateFunctionPointer(
		device, "vkCmdDrawMeshTasksIndirectEXT", VkExtMeshShader::s_vkCmdDrawMeshTasksIndirectEXT
	);
}
