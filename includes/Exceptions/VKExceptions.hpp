#ifndef VK_EXCEPTIONS_HPP_
#define VK_EXCEPTIONS_HPP_
#include <Exception.hpp>
#include <vulkan/vulkan.hpp>

class VKException final : public Exception {
public:
	VKException(int line, const char* file, VkResult errorCode) noexcept;

	[[nodiscard]]
	std::string GetErrorString() const noexcept;

	void GenerateWhatBuffer() noexcept override;

	[[nodiscard]]
	const char* what() const noexcept override;
	[[nodiscard]]
	const char* GetType() const noexcept override;

private:
	VkResult m_errorCode;
};
#endif
