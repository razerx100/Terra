#ifndef VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#define VIEWPORT_AND_SCISSOR_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <cstdint>
#include <optional>

class ViewportAndScissorManager {
public:
	struct Args {
		std::optional<std::uint32_t> width;
		std::optional<std::uint32_t> height;
	};

public:
	ViewportAndScissorManager(const Args& arguments);

	[[nodiscard]]
	const VkViewport* GetViewportRef() const noexcept;
	[[nodiscard]]
	const VkRect2D* GetScissorRef() const noexcept;

	void Resize(std::uint32_t width, std::uint32_t height) noexcept;

private:
	void ResizeViewport(std::uint32_t width, std::uint32_t height) noexcept;
	void ResizeScissor(std::uint32_t width, std::uint32_t height) noexcept;

private:
	VkViewport m_viewport;
	VkRect2D m_scissor;
};
#endif
