#ifndef VK_RENDER_PASS_HPP_
#define VK_RENDER_PASS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>
#include <span>
#include <utility>

class RenderPassBuilder
{
public:
	RenderPassBuilder()
		: m_subpassDesc{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS
		},
		m_subpassDependency{
			.srcSubpass    = VK_SUBPASS_EXTERNAL,
			.dstSubpass    = 0u,
			.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							| VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0u
		},
		m_depthAttachmentRefs{}, m_colourAttachmentRefs{}, m_attachments{},
		m_createInfo{
			.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.subpassCount    = 1u,
			.pSubpasses      = &m_subpassDesc,
			.dependencyCount = 1u,
			.pDependencies   = &m_subpassDependency
		} {}

	RenderPassBuilder& AddColourAttachment(VkFormat format) noexcept;
	RenderPassBuilder& AddDepthAttachment(VkFormat format) noexcept;

	RenderPassBuilder& Build() noexcept;

	[[nodiscard]]
	const VkRenderPassCreateInfo* GetRef() const noexcept { return &m_createInfo; }
	[[nodiscard]]
	VkRenderPassCreateInfo Get() const noexcept { return m_createInfo; }

private:
	void UpdatePointers() noexcept;

	// Might replace these with their respective builders if I need to use customised
	// attachments later on.
	static VkAttachmentDescription GetColourAttachment(VkFormat format) noexcept;
	static VkAttachmentDescription GetDepthAttachment(VkFormat format) noexcept;

private:
	VkSubpassDescription                 m_subpassDesc;
	VkSubpassDependency                  m_subpassDependency;
	std::vector<VkAttachmentReference>   m_depthAttachmentRefs;
	std::vector<VkAttachmentReference>   m_colourAttachmentRefs;
	std::vector<VkAttachmentDescription> m_attachments;
	VkRenderPassCreateInfo               m_createInfo;

public:
	RenderPassBuilder(const RenderPassBuilder& other) noexcept
		: m_subpassDesc{ other.m_subpassDesc }, m_subpassDependency{ other.m_subpassDependency },
		m_depthAttachmentRefs{ other.m_depthAttachmentRefs },
		m_colourAttachmentRefs{ other.m_colourAttachmentRefs },
		m_attachments{ other.m_attachments }, m_createInfo{ other.m_createInfo }
	{
		UpdatePointers();
	}
	RenderPassBuilder& operator=(const RenderPassBuilder& other) noexcept
	{
		m_subpassDesc          = other.m_subpassDesc;
		m_subpassDependency    = other.m_subpassDependency;
		m_depthAttachmentRefs  = other.m_depthAttachmentRefs;
		m_colourAttachmentRefs = other.m_colourAttachmentRefs;
		m_attachments          = other.m_attachments;
		m_createInfo           = other.m_createInfo;

		UpdatePointers();

		return *this;
	}

	RenderPassBuilder(RenderPassBuilder&& other) noexcept
		: m_subpassDesc{ other.m_subpassDesc }, m_subpassDependency{ other.m_subpassDependency },
		m_depthAttachmentRefs{ std::move(other.m_depthAttachmentRefs) },
		m_colourAttachmentRefs{ std::move(other.m_colourAttachmentRefs) },
		m_attachments{ std::move(other.m_attachments) }, m_createInfo{ other.m_createInfo }
	{
		UpdatePointers();
	}
	RenderPassBuilder& operator=(RenderPassBuilder&& other) noexcept
	{
		m_subpassDesc          = other.m_subpassDesc;
		m_subpassDependency    = other.m_subpassDependency;
		m_depthAttachmentRefs  = std::move(other.m_depthAttachmentRefs);
		m_colourAttachmentRefs = std::move(other.m_colourAttachmentRefs);
		m_attachments          = std::move(other.m_attachments);
		m_createInfo           = other.m_createInfo;

		UpdatePointers();

		return *this;
	}
};

class VKRenderPass
{
public:
	VKRenderPass(VkDevice device) noexcept : m_device{ device }, m_renderPass{ VK_NULL_HANDLE } {}
	~VKRenderPass() noexcept;

	void Create(const RenderPassBuilder& renderPassBuilder);

	void BeginPass(
		VkCommandBuffer graphicsCmdBuffer, VkFramebuffer frameBuffer, VkExtent2D swapchainExtent,
		std::span<VkClearValue> clearValues
	);

	// This doesn't really need to be here, but for completion's sake, there should be an end.
	static void EndPass(VkCommandBuffer graphicsCmdBuffer);

	[[nodiscard]]
	VkRenderPass Get() const noexcept { return m_renderPass; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice     m_device;
	VkRenderPass m_renderPass;

public:
	VKRenderPass(const VKRenderPass&) = delete;
	VKRenderPass& operator=(const VKRenderPass&) = delete;

	VKRenderPass(VKRenderPass&& other) noexcept
		: m_device{ other.m_device }, m_renderPass{ std::exchange(other.m_renderPass, VK_NULL_HANDLE) }
	{}
	VKRenderPass& operator=(VKRenderPass&& other) noexcept
	{
		SelfDestruct();

		m_device     = other.m_device;
		m_renderPass = std::exchange(other.m_renderPass, VK_NULL_HANDLE);

		return *this;
	}
};
#endif
