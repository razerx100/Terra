#ifndef __COMMON_PIPELINE_OBJECTS_HPP__
#define __COMMON_PIPELINE_OBJECTS_HPP__
#include <vulkan/vulkan.hpp>
#include <IModel.hpp>
#include <vector>
#include <span>

VkPipelineInputAssemblyStateCreateInfo GetInputAssemblyStateCreateInfo() noexcept;

VkPipelineViewportStateCreateInfo GetViewportStateCreateInfo() noexcept;

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
