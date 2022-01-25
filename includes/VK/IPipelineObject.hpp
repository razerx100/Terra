#ifndef __I_PIPELINE_OBJECT_HPP__
#define __I_PIPELINE_OBJECT_HPP__
#include <vulkan/vulkan.hpp>

class IPipelineObject {
public:
	virtual ~IPipelineObject() = default;

	virtual VkPipeline GetPipelineObject() const noexcept = 0;
};
#endif
