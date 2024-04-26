#ifndef PIPELINE_LAYOUT_HPP_
#define PIPELINE_LAYOUT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <VkDescriptorBuffer.hpp>

class PipelineLayout
{
public:
	PipelineLayout(VkDevice device)
		: m_device{ device }, m_pipelineLayout{ VK_NULL_HANDLE }, m_pushConstantOffset{ 0u } {}
	~PipelineLayout() noexcept;

	void AddPushConstantRange(VkShaderStageFlags shaderFlag, std::uint32_t rangeSize) noexcept;

	void Create(const std::vector<DescriptorSetLayout>& setLayouts);
	void Create(const VkDescriptorBuffer& descBuffer);
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
		: m_device{ other.m_device }, m_pipelineLayout{ other.m_pipelineLayout },
		m_pushConstantOffset{ other.m_pushConstantOffset },
		m_pushRanges{ std::move(other.m_pushRanges) }
	{
		other.m_pipelineLayout = VK_NULL_HANDLE;
	}
	PipelineLayout& operator=(PipelineLayout&& other) noexcept
	{
		SelfDestruct();

		m_device               = other.m_device;
		m_pipelineLayout       = other.m_pipelineLayout;
		m_pushConstantOffset   = other.m_pushConstantOffset;
		m_pushRanges           = std::move(other.m_pushRanges);
		other.m_pipelineLayout = VK_NULL_HANDLE;

		return *this;
	}
};
#endif
