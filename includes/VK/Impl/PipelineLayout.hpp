#ifndef __PIPELINE_LAYOUT_HPP__
#define __PIPELINE_LAYOUT_HPP__
#include <IPipelineLayout.hpp>

class PipelineLayout : public IPipelineLayout {
public:
	PipelineLayout(VkDevice device);
	~PipelineLayout() noexcept;

	void CreateLayout();

	VkPipelineLayout GetLayout() const noexcept override;

private:
	VkDevice m_deviceRef;
	VkPipelineLayout m_pipelineLayout;
	VkPipelineLayoutCreateInfo m_createInfo;
};
#endif
