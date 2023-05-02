#ifndef VK_EXTENSION_LOADER_HPP_
#define	VK_EXTENSION_LOADER_HPP_
#include <vulkan/vulkan.hpp>
#include <unordered_map>

class VkDeviceExtensionLoader {
public:
	void AddFunctionPTR(const std::string& name) noexcept;
	void QueryFunctionPTRs(VkDevice device) noexcept;

	template<typename T>
	void GetFunctionPointer(const std::string& name, T& outputPTR) const noexcept {
		outputPTR = nullptr;

		if (auto ptr = m_functionPtrMap.find(name); ptr != std::end(m_functionPtrMap))
			outputPTR = reinterpret_cast<T>(ptr->second);
	}

	template<typename T>
	void AddAndGetFunctionPointer(
		VkDevice device, const std::string& name, T& outputPTR
	) noexcept {
		PFN_vkVoidFunction funcPtr = vkGetDeviceProcAddr(device, name.c_str());

		m_functionPtrMap.try_emplace(name, funcPtr);

		outputPTR = reinterpret_cast<T>(funcPtr);
	}

private:
	std::unordered_map<std::string, PFN_vkVoidFunction> m_functionPtrMap;
};
#endif
