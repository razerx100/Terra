#ifndef VK_SHADER_HPP_
#define VK_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

class VkShader {
public:
	VkShader(VkDevice device);
	~VkShader() noexcept;

	void CreateShader(VkDevice device, const std::wstring& fileName);

	[[nodiscard]]
	VkShaderModule GetShaderModule() const noexcept;

private:
	[[nodiscard]]
	std::vector<char> LoadBinary(const std::wstring& fileName);

	void CreateShaderModule(VkDevice device, void const* binary, size_t binarySize);

private:
	VkDevice m_deviceRef;
	VkShaderModule m_pBinary;
};
#endif
