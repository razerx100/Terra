#ifndef DESCRIPTOR_SET_LAYOUT_HPP_
#define DESCRIPTOR_SET_LAYOUT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <utility>

class DescriptorSetLayout
{
public:
	DescriptorSetLayout(VkDevice device)
		: m_device{ device }, m_layout{ VK_NULL_HANDLE }, m_layoutBindings{}, m_layoutBindingFlags{}
	{}
	~DescriptorSetLayout() noexcept;

	void Create();

	void AddBinding(
		std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
		VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags
	) noexcept;

	void UpdateBinding(
		std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
		VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags
	) noexcept;

	[[nodiscard]]
	VkDescriptorSetLayout Get() const noexcept { return m_layout; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice                                  m_device;
	VkDescriptorSetLayout                     m_layout;
	std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;
	std::vector<VkDescriptorBindingFlags>     m_layoutBindingFlags;

public:
	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

	DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
		: m_device{ other.m_device }, m_layout{ std::exchange(other.m_layout, VK_NULL_HANDLE) },
		m_layoutBindings{ std::move(other.m_layoutBindings) },
		m_layoutBindingFlags{ std::move(other.m_layoutBindingFlags) }
	{}

	DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept
	{
		SelfDestruct();

		m_device             = other.m_device;
		m_layout             = std::exchange(other.m_layout, VK_NULL_HANDLE);
		m_layoutBindings     = std::move(other.m_layoutBindings);
		m_layoutBindingFlags = std::move(other.m_layoutBindingFlags);

		return *this;
	}
};
#endif
