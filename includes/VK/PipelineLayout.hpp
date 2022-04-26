#ifndef PIPELINE_LAYOUT_HPP_
#define PIPELINE_LAYOUT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class PipelineLayout {
public:
	PipelineLayout(VkDevice device);
	~PipelineLayout() noexcept;

	void AddPushConstantRange(
		VkShaderStageFlags shaderFlag, std::uint32_t rangeSize
	) noexcept;

	void CreateLayout(
		const std::vector<VkDescriptorSetLayout>& setLayout
	);

	[[nodiscard]]
	VkPipelineLayout GetLayout() const noexcept;

private:
	VkDevice m_deviceRef;
	VkPipelineLayout m_pipelineLayout;
	std::uint32_t m_pushConstantOffset;
	std::vector<VkPushConstantRange> m_pushRanges;
};
#endif
