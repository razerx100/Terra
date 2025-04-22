#ifndef VK_VERTEX_LAYOUT_HPP_
#define VK_VERTEX_LAYOUT_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

class VertexLayout
{
public:
	VertexLayout(std::uint32_t binding = 0u)
		: m_bindingDesc{
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
		},
		m_createInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
		},
		m_attrDescs{}, m_binding{ binding }, m_vertexOffset{ 0u }, m_position{ 0u }
	{}

	[[nodiscard]]
	VertexLayout& AddInput(VkFormat format, std::uint32_t sizeInBytes) noexcept;
	[[nodiscard]]
	VertexLayout& InitLayout() noexcept;

	[[nodiscard]]
	const VkPipelineVertexInputStateCreateInfo* GetRef() const noexcept { return &m_createInfo; }
	[[nodiscard]]
	VkPipelineVertexInputStateCreateInfo Get() const noexcept { return m_createInfo; }

private:
	void UpdatePointers() noexcept;

private:
	VkVertexInputBindingDescription                m_bindingDesc;
	VkPipelineVertexInputStateCreateInfo           m_createInfo;
	std::vector<VkVertexInputAttributeDescription> m_attrDescs;

	// The binding is like Space in HLSL.
	std::uint32_t m_binding;
	std::uint32_t m_vertexOffset;
	std::uint32_t m_position;

public:
	VertexLayout(const VertexLayout& other) noexcept
		: m_bindingDesc{ other.m_bindingDesc }, m_createInfo{ other.m_createInfo },
		m_attrDescs{ other.m_attrDescs }, m_binding{ other.m_binding },
		m_vertexOffset{ other.m_vertexOffset }, m_position{ other.m_position }
	{
		UpdatePointers();
	}
	VertexLayout& operator=(const VertexLayout& other) noexcept
	{
		m_createInfo   = other.m_createInfo;
		m_bindingDesc  = other.m_bindingDesc;
		m_attrDescs    = other.m_attrDescs;
		m_binding      = other.m_binding;
		m_vertexOffset = other.m_vertexOffset;
		m_position     = other.m_position;

		UpdatePointers();

		return *this;
	}

	VertexLayout(VertexLayout&& other) noexcept
		: m_bindingDesc{ other.m_bindingDesc }, m_createInfo{ other.m_createInfo },
		m_attrDescs{ std::move(other.m_attrDescs) }, m_binding{ other.m_binding },
		m_vertexOffset{ other.m_vertexOffset }, m_position{ other.m_position }
	{
		UpdatePointers();
	}
	VertexLayout& operator=(VertexLayout&& other) noexcept
	{
		m_createInfo   = other.m_createInfo;
		m_bindingDesc  = other.m_bindingDesc;
		m_attrDescs    = std::move(other.m_attrDescs);
		m_binding      = other.m_binding;
		m_vertexOffset = other.m_vertexOffset;
		m_position     = other.m_position;

		UpdatePointers();

		return *this;
	}
};
#endif
