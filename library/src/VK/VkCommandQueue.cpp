#include <VkCommandQueue.hpp>

// Command Buffer
VKCommandBuffer::VKCommandBuffer() : m_commandBuffer{VK_NULL_HANDLE} {}
VKCommandBuffer::VKCommandBuffer(VkDevice device, VkCommandPool commandPool) : VKCommandBuffer{}
{
	Create(device, commandPool);
}

void VKCommandBuffer::Create(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocateInfo{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = commandPool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1u
	};

	vkAllocateCommandBuffers(device, &allocateInfo, &m_commandBuffer);
}

VKCommandBuffer& VKCommandBuffer::Reset() noexcept
{
	VkCommandBufferBeginInfo beginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::Close() noexcept
{
	vkEndCommandBuffer(m_commandBuffer);

	return *this;
}

VKCommandBuffer& VKCommandBuffer::Copy(
	const Buffer& src, const Buffer& dst, const BufferToBufferCopyBuilder& builder
) noexcept {
	vkCmdCopyBuffer(m_commandBuffer, src.Get(), dst.Get(), 1u, builder.GetPtr());

	return *this;
}

VKCommandBuffer& VKCommandBuffer::Copy(const Buffer& src, const VkTextureView& dst) noexcept
{
	return *this;
}

// Command Queue
VkCommandQueue::~VkCommandQueue() noexcept
{
	SelfDestruct();
}

void VkCommandQueue::SelfDestruct() noexcept
{
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
}

void VkCommandQueue::CreateBuffers(std::uint32_t bufferCount)
{
	VkCommandPoolCreateInfo poolInfo{
		.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = m_queueIndex
	};

	vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);

	for (std::uint32_t _ = 0u; _ < bufferCount; ++_)
		m_commandBuffers.emplace_back(m_device, m_commandPool);
}
