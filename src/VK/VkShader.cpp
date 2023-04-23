#include <VkShader.hpp>
#include <fstream>

VkShader::VkShader(VkDevice device)
	: m_deviceRef{ device }, m_pBinary{ VK_NULL_HANDLE } {}

VkShader::~VkShader() noexcept {
	vkDestroyShaderModule(m_deviceRef, m_pBinary, nullptr);
}

void VkShader::CreateShader(VkDevice device, const std::wstring& fileName) {
	std::vector<char> byteCode = LoadBinary(fileName);

	CreateShaderModule(device, std::data(byteCode), std::size(byteCode));
}

void VkShader::CreateShaderModule(
	VkDevice device, void const* binary, size_t binarySize
) {
	VkShaderModuleCreateInfo createInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = binarySize,
		.pCode = static_cast<std::uint32_t const*>(binary)
	};

	vkCreateShaderModule(device, &createInfo, nullptr, &m_pBinary);
}

VkShaderModule VkShader::GetShaderModule() const noexcept {
	return m_pBinary;
}

std::vector<char> VkShader::LoadBinary(const std::wstring& fileName) {
	std::ifstream shader(fileName.c_str(), std::ios_base::binary | std::ios_base::ate);

	const size_t shaderSize = static_cast<size_t>(shader.tellg());
	shader.seekg(0u);

	std::vector<char> byteCode(shaderSize);
	shader.read(std::data(byteCode), static_cast<std::streamsize>(shaderSize));

	return byteCode;
}
