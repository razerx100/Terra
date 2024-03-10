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

void VKSemaphore::Create(bool timeline/* = false */, std::uint64_t initialValue/* = 0u */)
{
	VkSemaphoreTypeCreateInfo typeCreateInfo{
		.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
		.semaphoreType = VK_SEMAPHORE_TYPE_BINARY,
		.initialValue  = 0u
	};

	VkSemaphoreCreateInfo semaphoreInfo{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		.pNext = &typeCreateInfo
	};

	if (timeline)
	{
		typeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		typeCreateInfo.initialValue  = initialValue;
	}

	vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_syncObj);
}

void VKSemaphore::Signal(std::uint64_t signalValue /* = 1u */) const noexcept
{
	VkSemaphoreSignalInfo signalInfo{
		.sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
		.semaphore = m_syncObj,
		.value     = signalValue
	};

	vkSignalSemaphore(m_device, &signalInfo);
}

void VKSemaphore::Wait(std::uint64_t waitValue /* = 1u */) const noexcept
{
	VkSemaphoreWaitInfo waitInfo{
		.sType          = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.semaphoreCount = 1u,
		.pSemaphores    = &m_syncObj,
		.pValues        = &waitValue
	};

	vkWaitSemaphores(m_device, &waitInfo, std::numeric_limits<std::uint64_t>::max());
}

std::uint64_t VKSemaphore::GetCurrentValue() const noexcept
{
	std::uint64_t currentValue = 0u;
	vkGetSemaphoreCounterValue(m_device, m_syncObj, &currentValue);

	return currentValue;
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
