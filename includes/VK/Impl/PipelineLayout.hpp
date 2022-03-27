#ifndef __PIPELINE_LAYOUT_HPP__
#define __PIPELINE_LAYOUT_HPP__
#include <IPipelineLayout.hpp>
#include <vector>

class PipelineLayout : public IPipelineLayout {
public:
	PipelineLayout(VkDevice device);
	~PipelineLayout() noexcept;

	void AddPushConstantRange(
		VkShaderStageFlags shaderFlag, std::uint32_t rangeSize
	) noexcept;

	void CreateLayout();

	VkPipelineLayout GetLayout() const noexcept override;

private:
	VkDevice m_deviceRef;
	VkPipelineLayout m_pipelineLayout;
	std::uint32_t m_pushConstantOffset;
	std::vector<VkPushConstantRange> m_pushRanges;
};
#endif
