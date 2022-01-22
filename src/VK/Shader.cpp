#include <Shader.hpp>
#include <fstream>
#include <VKThrowMacros.hpp>

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

	size_t shaderSize = static_cast<size_t>(shader.tellg());
	shader.seekg(0u);

	std::vector<char> byteCode(shaderSize);
	shader.read(byteCode.data(), shaderSize);

	return byteCode;
}
