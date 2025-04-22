#ifndef VK_EXTERNAL_FORMAT_MAP_HPP_
#define VK_EXTERNAL_FORMAT_MAP_HPP_
#include <vulkan/vulkan.hpp>
#include <ExternalFormat.hpp>
#include <ExternalPipeline.hpp>

namespace Terra
{
[[nodiscard]]
VkFormat GetVkFormat(ExternalFormat format) noexcept;
[[nodiscard]]
ExternalFormat GetExternalFormat(VkFormat format) noexcept;

[[nodiscard]]
VkBlendFactor GetVkBlendFactor(ExternalBlendFactor factor) noexcept;
[[nodiscard]]
VkBlendOp GetVkBlendOP(ExternalBlendOP op) noexcept;
[[nodiscard]]
VkPipelineColorBlendAttachmentState GetVkBlendState(const ExternalBlendState& blendState) noexcept;
}
#endif
