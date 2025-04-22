#include <VkExtensionManager.hpp>
#include <TerraException.hpp>
#include <array>

namespace Terra
{
// Device extension names.
static std::array deviceExtensionNameMap
{
	"VK_EXT_mesh_shader",
	"VK_KHR_swapchain",
	"VK_EXT_memory_budget",
	"VK_EXT_descriptor_buffer"
};

void VkDeviceExtensionManager::PopulateExtensionFunctions(VkDevice device) const noexcept
{
	if (m_extensions.test(static_cast<size_t>(DeviceExtension::VkExtMeshShader)))
		PopulateVkExtMeshShader(device);

	if (m_extensions.test(static_cast<size_t>(DeviceExtension::VkExtDescriptorBuffer)))
		PopulateVkExtDescriptorBuffer(device);
}

void VkDeviceExtensionManager::AddExtensionName(size_t extensionIndex) noexcept
{
	m_extensionNames.emplace_back(deviceExtensionNameMap.at(extensionIndex));
}

// Instance extension names.
static std::array instanceExtensionNameMap
{
	"VK_EXT_debug_utils",
	"VK_KHR_surface",
	"VK_KHR_win32_surface",
	"VK_KHR_display",
	"VK_KHR_external_memory_capabilities"
};

void VkInstanceExtensionManager::PopulateExtensionFunctions(VkInstance instance) const noexcept
{
	if (m_extensions.test(static_cast<size_t>(InstanceExtension::VkExtDebugUtils)))
		PopulateVkExtDebugUtils(instance);
}

void VkInstanceExtensionManager::AddExtensionName(size_t extensionIndex) noexcept
{
	m_extensionNames.emplace_back(instanceExtensionNameMap.at(extensionIndex));
}

// Boring function pointer population functions
void VkInstanceExtensionManager::PopulateVkExtDebugUtils(VkInstance instance) noexcept
{
	using namespace VkInstanceExtension;

	PopulateFunctionPointer(
		instance, "vkCmdBeginDebugUtilsLabelEXT", VkExtDebugUtils::s_vkCmdBeginDebugUtilsLabelEXT
	);
	PopulateFunctionPointer(
		instance, "vkCmdEndDebugUtilsLabelEXT", VkExtDebugUtils::s_vkCmdEndDebugUtilsLabelEXT
	);
	PopulateFunctionPointer(
		instance, "vkCmdInsertDebugUtilsLabelEXT", VkExtDebugUtils::s_vkCmdInsertDebugUtilsLabelEXT
	);
	PopulateFunctionPointer(
		instance, "vkCreateDebugUtilsMessengerEXT", VkExtDebugUtils::s_vkCreateDebugUtilsMessengerEXT
	);
	PopulateFunctionPointer(
		instance, "vkDestroyDebugUtilsMessengerEXT", VkExtDebugUtils::s_vkDestroyDebugUtilsMessengerEXT
	);
	PopulateFunctionPointer(
		instance, "vkQueueBeginDebugUtilsLabelEXT", VkExtDebugUtils::s_vkQueueBeginDebugUtilsLabelEXT
	);
	PopulateFunctionPointer(
		instance, "vkQueueEndDebugUtilsLabelEXT", VkExtDebugUtils::s_vkQueueEndDebugUtilsLabelEXT
	);
	PopulateFunctionPointer(
		instance, "vkQueueInsertDebugUtilsLabelEXT", VkExtDebugUtils::s_vkQueueInsertDebugUtilsLabelEXT
	);
	PopulateFunctionPointer(
		instance, "vkSetDebugUtilsObjectNameEXT", VkExtDebugUtils::s_vkSetDebugUtilsObjectNameEXT
	);
	PopulateFunctionPointer(
		instance, "vkSetDebugUtilsObjectTagEXT", VkExtDebugUtils::s_vkSetDebugUtilsObjectTagEXT
	);
	PopulateFunctionPointer(
		instance, "vkSubmitDebugUtilsMessageEXT", VkExtDebugUtils::s_vkSubmitDebugUtilsMessageEXT
	);
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
}
