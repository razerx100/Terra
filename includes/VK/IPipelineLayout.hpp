#ifndef __I_PIPELINE_LAYOUT_HPP__
#define __I_PIPELINE_LAYOUT_HPP__
#include <vulkan/vulkan.hpp>

class IPipelineLayout {
public:
	virtual ~IPipelineLayout() = default;

	[[nodiscard]]
	virtual VkPipelineLayout GetLayout() const noexcept = 0;
};
#endif
