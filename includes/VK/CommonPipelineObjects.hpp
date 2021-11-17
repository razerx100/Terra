#ifndef __COMMON_PIPELINE_OBJECTS_HPP__
#define __COMMON_PIPELINE_OBJECTS_HPP__
#include <vulkan/vulkan.hpp>
#include <vector>

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
	const VkExtent2D& extent
) noexcept;

void PopulateViewportStateCreateInfo(
	VkPipelineViewportStateCreateInfo& createInfo,
	const std::vector<VkViewport>& viewports,
	const std::vector<VkRect2D>& scissors
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
	const std::vector<VkPipelineColorBlendAttachmentState>& attachmentStates
) noexcept;

void PopulateDynamicStateCreateInfo(
	VkPipelineDynamicStateCreateInfo& createInfo,
	const std::vector<VkDynamicState>& dynamicStates
) noexcept;
#endif
