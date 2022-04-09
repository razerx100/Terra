#ifndef __I_SHADER_HPP__
#define __I_SHADER_HPP__
#include <vulkan/vulkan.hpp>

class IShader {
public:
	virtual ~IShader() = default;

	[[nodiscard]]
	virtual VkShaderModule GetByteCode() const noexcept = 0;
};
#endif
