#ifndef VK_SHADER_HPP_
#define VK_SHADER_HPP_
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include <fstream>

class VkShader
{
public:
	VkShader(VkDevice device) : m_device{ device }, m_shaderBinary{ VK_NULL_HANDLE } {}
	~VkShader() noexcept;

	[[nodiscard]]
	bool Create(const std::wstring& fileName);

	[[nodiscard]]
	VkShaderModule Get() const noexcept { return m_shaderBinary; }

private:
	void SelfDestruct() noexcept;

private:
	[[nodiscard]]
	static std::vector<char> LoadBinary(std::ifstream& shader);

	void CreateShaderModule(VkDevice device, void const* binary, size_t binarySize);

private:
	VkDevice       m_device;
	VkShaderModule m_shaderBinary;

public:
	VkShader(const VkShader&) = delete;
	VkShader& operator=(const VkShader&) = delete;

	VkShader(VkShader&& other) noexcept
		: m_device{ other.m_device }, m_shaderBinary{ other.m_shaderBinary }
	{
		other.m_shaderBinary = VK_NULL_HANDLE;
	}
	VkShader& operator=(VkShader&& other) noexcept
	{
		SelfDestruct();

		m_device             = other.m_device;
		m_shaderBinary       = other.m_shaderBinary;
		other.m_shaderBinary = VK_NULL_HANDLE;

		return *this;
	}
};
#endif
