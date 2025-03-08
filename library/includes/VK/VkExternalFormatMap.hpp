#ifndef VK_EXTERNAL_FORMAT_MAP_HPP_
#define VK_EXTERNAL_FORMAT_MAP_HPP_
#include <vulkan/vulkan.hpp>
#include <ExternalFormat.hpp>

[[nodiscard]]
VkFormat GetVkFormat(ExternalFormat format) noexcept;
#endif
