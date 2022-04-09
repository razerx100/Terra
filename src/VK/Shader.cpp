#include <Shader.hpp>
#include <fstream>
#include <VKThrowMacros.hpp>

Shader::Shader(VkDevice device)
	: m_deviceRef(device), m_pBinary(VK_NULL_HANDLE) {}

Shader::~Shader() noexcept {
	vkDestroyShaderModule(m_deviceRef, m_pBinary, nullptr);
}

void Shader::CreateShader(VkDevice device, const std::string& fileName) {
	std::vector<char> byteCode = LoadBinary(fileName);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = byteCode.size();
	createInfo.pCode = reinterpret_cast<std::uint32_t*>(byteCode.data());

	VkResult result;
	VK_THROW_FAILED(result,
		vkCreateShaderModule(device, &createInfo, nullptr, &m_pBinary)
	);
}

VkShaderModule Shader::GetByteCode() const noexcept {
	return m_pBinary;
}

std::vector<char> Shader::LoadBinary(const std::string& fileName) {
	std::ifstream shader(fileName.c_str(), std::ios_base::binary | std::ios_base::ate);

	const size_t shaderSize = static_cast<size_t>(shader.tellg());
	shader.seekg(0u);

	std::vector<char> byteCode(shaderSize);
	shader.read(byteCode.data(), static_cast<std::streamsize>(shaderSize));

	return byteCode;
}
