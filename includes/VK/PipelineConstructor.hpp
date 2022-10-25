#ifndef PIPELINE_CONSTRUCTOR_HPP_
#define PIPELINE_CONSTRUCTOR_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <VKPipelineObject.hpp>
#include <PipelineLayout.hpp>

[[nodiscard]]
std::unique_ptr<PipelineLayout> CreateGraphicsPipelineLayout(
	VkDevice device, std::uint32_t layoutCount, VkDescriptorSetLayout const* setLayouts
) noexcept;
[[nodiscard]]
std::unique_ptr<VkPipelineObject> CreateGraphicsPipeline(
	VkDevice device, VkPipelineLayout graphicsLayout, VkRenderPass renderPass,
	const std::wstring& shaderPath
) noexcept;
#endif
