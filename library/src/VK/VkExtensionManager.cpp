#include <VkExtensionManager.hpp>
#include <Exception.hpp>
#include <array>

// Extension Manager
static std::array extensionNameMap
{
	"VK_EXT_mesh_shader",
	"VK_KHR_swapchain",
	"VK_EXT_memory_budget",
	"VK_EXT_descriptor_buffer"
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

	if (m_extensions.test(static_cast<size_t>(DeviceExtension::VkExtDescriptorBuffer)))
		PopulateVkExtDescriptorBuffer(device);
}

const std::vector<const char*>& VkDeviceExtensionManager::GetExtensionNames() const noexcept
{
	return m_extensionNames;
}

bool VkDeviceExtensionManager::IsExtensionActive(DeviceExtension extension) const noexcept
{
	return m_extensions.test(static_cast<size_t>(extension));
}

std::vector<DeviceExtension> VkDeviceExtensionManager::GetActiveExtensions() const noexcept
{
	std::vector<DeviceExtension> activeExtensions{};

	const auto extensionCount = static_cast<size_t>(DeviceExtension::None);

	for (size_t index = 0u; index < extensionCount; ++index)
		if (m_extensions.test(index))
			activeExtensions.emplace_back(static_cast<DeviceExtension>(index));

	return activeExtensions;
}

// Boring function pointer population functions
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

void VkDeviceExtensionManager::PopulateVkExtDescriptorBuffer(VkDevice device) noexcept
{
	using namespace VkDeviceExtension;

	PopulateFunctionPointer(
		device, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT",
		VkExtDescriptorBuffer::s_vkCmdBindDescriptorBufferEmbeddedSamplersEXT
	);
	PopulateFunctionPointer(
		device, "vkCmdBindDescriptorBuffersEXT", VkExtDescriptorBuffer::s_vkCmdBindDescriptorBuffersEXT
	);
	PopulateFunctionPointer(
		device, "vkCmdSetDescriptorBufferOffsetsEXT",
		VkExtDescriptorBuffer::s_vkCmdSetDescriptorBufferOffsetsEXT
	);
	PopulateFunctionPointer(
		device, "vkGetBufferOpaqueCaptureDescriptorDataEXT",
		VkExtDescriptorBuffer::s_vkGetBufferOpaqueCaptureDescriptorDataEXT
	);
	PopulateFunctionPointer(
		device, "vkGetDescriptorEXT", VkExtDescriptorBuffer::s_vkGetDescriptorEXT
	);
	PopulateFunctionPointer(
		device, "vkGetDescriptorSetLayoutBindingOffsetEXT",
		VkExtDescriptorBuffer::s_vkGetDescriptorSetLayoutBindingOffsetEXT
	);
	PopulateFunctionPointer(
		device, "vkGetDescriptorSetLayoutSizeEXT",
		VkExtDescriptorBuffer::s_vkGetDescriptorSetLayoutSizeEXT
	);
	PopulateFunctionPointer(
		device, "vkGetImageOpaqueCaptureDescriptorDataEXT",
		VkExtDescriptorBuffer::s_vkGetImageOpaqueCaptureDescriptorDataEXT
	);
	PopulateFunctionPointer(
		device, "vkGetImageViewOpaqueCaptureDescriptorDataEXT",
		VkExtDescriptorBuffer::s_vkGetImageViewOpaqueCaptureDescriptorDataEXT
	);
	PopulateFunctionPointer(
		device, "vkGetSamplerOpaqueCaptureDescriptorDataEXT",
		VkExtDescriptorBuffer::s_vkGetSamplerOpaqueCaptureDescriptorDataEXT
	);
	PopulateFunctionPointer(
		device, "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT",
		VkExtDescriptorBuffer::s_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT
	);
}
