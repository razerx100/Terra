#ifndef VK_PIPELINE_LAYOUT_HPP_
#define VK_PIPELINE_LAYOUT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkDescriptorSetLayout.hpp>

class PipelineLayout
{
public:
	PipelineLayout(VkDevice device)
		: m_device{ device }, m_pipelineLayout{ VK_NULL_HANDLE }, m_pushConstantOffset{ 0u }
	{}
	~PipelineLayout() noexcept;

	void AddPushConstantRange(VkShaderStageFlags shaderFlag, std::uint32_t rangeSize) noexcept;

	void Create(const std::vector<DescriptorSetLayout>& setLayouts);
	void Create(const std::vector<VkDescriptorSetLayout>& setLayouts);
	void Create(const DescriptorSetLayout& setLayout);
	void Create(VkDescriptorSetLayout const* setLayouts, std::uint32_t layoutCount);

	[[nodiscard]]
	VkPipelineLayout Get() const noexcept { return m_pipelineLayout; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice                         m_device;
	VkPipelineLayout                 m_pipelineLayout;
	std::uint32_t                    m_pushConstantOffset;
	std::vector<VkPushConstantRange> m_pushRanges;

public:
	PipelineLayout(const PipelineLayout&) = delete;
	PipelineLayout& operator=(const PipelineLayout&) = delete;

	PipelineLayout(PipelineLayout&& other) noexcept
		: m_device{ other.m_device },
		m_pipelineLayout{ std::exchange(other.m_pipelineLayout, VK_NULL_HANDLE) },
		m_pushConstantOffset{ other.m_pushConstantOffset },
		m_pushRanges{ std::move(other.m_pushRanges) }
	{}
	PipelineLayout& operator=(PipelineLayout&& other) noexcept
	{
		SelfDestruct();

		m_device             = other.m_device;
		m_pipelineLayout     = std::exchange(other.m_pipelineLayout, VK_NULL_HANDLE);
		m_pushConstantOffset = other.m_pushConstantOffset;
		m_pushRanges         = std::move(other.m_pushRanges);

		return *this;
	}
};
#endif
