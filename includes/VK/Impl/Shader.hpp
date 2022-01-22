#ifndef __SHADER_HPP__
#define __SHADER_HPP__
#include <IShader.hpp>
#include <string>
#include <vector>

class Shader : public IShader {
public:
	void CreateShader(VkDevice device, const std::string& fileName);

	VkShaderModule GetByteCode() const noexcept override;

private:
	std::vector<char> LoadBinary(const std::string& fileName);

private:
	VkShaderModule m_pBinary;
};
#endif
