#include <VkExtensionLoader.hpp>
#include <Exception.hpp>

void VkDeviceExtensionLoader::AddFunctionPTR(const std::string& name) noexcept{
	m_functionPtrMap.try_emplace(name, nullptr);
}

void VkDeviceExtensionLoader::QueryFunctionPTRs(VkDevice device) noexcept {
	for (auto& [name, ptr] : m_functionPtrMap)
		ptr = vkGetDeviceProcAddr(device, name.c_str());
}
