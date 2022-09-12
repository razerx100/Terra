#ifndef SHADER_HPP_
#define SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>

class Shader {
public:
	Shader(VkDevice device);
	~Shader() noexcept;

	void CreateShader(VkDevice device, const std::wstring& fileName);

	[[nodiscard]]
	VkShaderModule GetByteCode() const noexcept;

private:
	std::vector<char> LoadBinary(const std::wstring& fileName);

private:
	VkDevice m_deviceRef;
	VkShaderModule m_pBinary;
};
#endif
