#include <VkShader.hpp>
#include <TerraException.hpp>

VkShader::~VkShader() noexcept
{
	SelfDestruct();
}

void VkShader::SelfDestruct() noexcept
{
	vkDestroyShaderModule(m_device, m_shaderBinary, nullptr);
}

bool VkShader::Create(const std::wstring& fileName)
{
	std::ifstream shader{ fileName.c_str(), std::ios_base::binary | std::ios_base::ate };

	bool success = false;

	if (shader.is_open())
	{
		std::vector<char> byteCode = LoadBinary(shader);

		CreateShaderModule(m_device, std::data(byteCode), std::size(byteCode));

		success = true;
	}

	return success;
}

void VkShader::CreateShaderModule(
	VkDevice device, void const* binary, size_t binarySize
) {
	VkShaderModuleCreateInfo createInfo{
		.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = binarySize,
		.pCode    = static_cast<std::uint32_t const*>(binary)
	};

	vkCreateShaderModule(device, &createInfo, nullptr, &m_shaderBinary);
}

std::vector<char> VkShader::LoadBinary(std::ifstream& shader)
{
	const auto shaderSize = static_cast<size_t>(shader.tellg());
	shader.seekg(0u);

	std::vector<char> byteCode{};
	byteCode.resize(shaderSize);
	shader.read(std::data(byteCode), static_cast<std::streamsize>(shaderSize));

	return byteCode;
}
