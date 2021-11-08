#ifndef __VK_EXCEPTIONS_HPP__
#define __VK_EXCEPTIONS_HPP__
#include <Exception.hpp>
#include <vulkan/vulkan.hpp>

class VKException : public Exception {
public:
	VKException(int line, const char* file, VkResult errorCode) noexcept;

	std::string GetErrorString() const noexcept;

	void GenerateWhatBuffer() noexcept override;
	const char* what() const noexcept override;
	const char* GetType() const noexcept override;

private:
	VkResult m_errorCode;
};
#endif
