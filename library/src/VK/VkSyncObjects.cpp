#include <VkSyncObjects.hpp>
#include <limits>

// VK Semaphore
VKSemaphore::~VKSemaphore() noexcept
{
	SelfDestruct();
}

void VKSemaphore::SelfDestruct() noexcept
{
	vkDestroySemaphore(m_device, m_syncObj, nullptr);
}

void VKSemaphore::Create()
{
	VkSemaphoreCreateInfo semaphoreInfo{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_syncObj);
}

// VK Fence
VKFence::~VKFence() noexcept
{
	SelfDestruct();
}

void VKFence::SelfDestruct() noexcept
{
	vkDestroyFence(m_device, m_syncObj, nullptr);
}

void VKFence::Create(bool signaled)
{
	VkFenceCreateInfo fenceInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0u
	};

	vkCreateFence(m_device, &fenceInfo, nullptr, &m_syncObj);
}

void VKFence::Wait() const
{
	vkWaitForFences(m_device, 1u, &m_syncObj, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
}

void VKFence::Reset() const
{
	vkResetFences(m_device, 1u, &m_syncObj);
}

// VK Multi Semaphore
VkMultiSemaphore::VkMultiSemaphore(VkDevice device, size_t semaphoreCount) : VkMultiSyncObj{ device }
{
	for (size_t _ = 0u; _ < semaphoreCount; ++_)
	{
		VKSemaphore semaphore{ device };
		semaphore.Create();
		m_syncObjs.emplace_back(std::move(semaphore));
	}
}

// VK Multi Semaphore
VkMultiFence::VkMultiFence(VkDevice device, size_t fenceCount, bool signaled) : VkMultiSyncObj{ device }
{
	assert(fenceCount != 0u && "Fence Count can't be zero");

	{
		VKFence nonSignaledFence{ device };
		nonSignaledFence.Create(false);
		m_syncObjs.emplace_back(std::move(nonSignaledFence));
	}

	--fenceCount;

	for (size_t _ = 0u; _ < fenceCount; ++_)
	{
		VKFence signaledFence{ device };
		signaledFence.Create(signaled);
		m_syncObjs.emplace_back(std::move(signaledFence));
	}
}

void VkMultiFence::WaitForFrontFence() const noexcept
{
	for (const VKFence& fence : m_syncObjs)
		fence.Wait();
}

void VkMultiFence::ResetFrontFence() const noexcept
{
	for (const VKFence& fence : m_syncObjs)
		fence.Reset();
}
