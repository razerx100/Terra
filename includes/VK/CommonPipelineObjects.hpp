#ifndef __COMMON_PIPELINE_OBJECTS_HPP__
#define __COMMON_PIPELINE_OBJECTS_HPP__
#include <vulkan/vulkan.hpp>
#include <span>

void PopulateVertexInputStateCreateInfo(
	VkPipelineVertexInputStateCreateInfo& createInfo
) noexcept;

void PopulateInputAssemblyStateCreateInfo(
	VkPipelineInputAssemblyStateCreateInfo& createInfo
) noexcept;

void PopulateViewport(
	VkViewport& viewport,
	std::uint32_t width, std::uint32_t height
) noexcept;

void PopulateScissorRect(
	VkRect2D& scissor,
	std::uint32_t width, std::uint32_t height
) noexcept;

void PopulateViewportStateCreateInfo(
	VkPipelineViewportStateCreateInfo& createInfo,
	const std::span<VkViewport> viewports,
	const std::span<VkRect2D> scissors
) noexcept;

void PopulateRasterizationStateCreateInfo(
	VkPipelineRasterizationStateCreateInfo& createInfo
) noexcept;

void PopulateMultisampleStateCreateInfo(
	VkPipelineMultisampleStateCreateInfo& createInfo
) noexcept;

void PopulateColorBlendAttachmentState(
	VkPipelineColorBlendAttachmentState& createInfo
) noexcept;

void PopulateColorBlendStateCreateInfo(
	VkPipelineColorBlendStateCreateInfo& createInfo,
	const std::span<VkPipelineColorBlendAttachmentState> attachmentStates
) noexcept;

void PopulateDynamicStateCreateInfo(
	VkPipelineDynamicStateCreateInfo& createInfo,
	const std::span<VkDynamicState> dynamicStates
) noexcept;
#endif
