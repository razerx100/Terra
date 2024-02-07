#ifndef VK_SYNC_OBJECTS_HPP_
#define VK_SYNC_OBJECTS_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

template<typename ObjType>
class VkSyncObj
{
public:
	VkSyncObj(VkDevice device) : m_device{ device }, m_syncObj{ VK_NULL_HANDLE } {}
	virtual ~VkSyncObj() = default;

	[[nodiscard]]
	ObjType Get() const noexcept { return m_syncObj; }

protected:
	VkDevice m_device;
	ObjType  m_syncObj;

public:
	VkSyncObj(const VkSyncObj&) = delete;
	VkSyncObj& operator=(const VkSyncObj&) = delete;

	VkSyncObj(VkSyncObj&& other) noexcept : m_device{ other.m_device }, m_syncObj{ other.m_syncObj }
	{
		other.m_syncObj = VK_NULL_HANDLE;
	}
	VkSyncObj& operator=(VkSyncObj&& other) noexcept
	{
		m_device        = other.m_device;
		m_syncObj       = other.m_syncObj;
		other.m_syncObj = VK_NULL_HANDLE;

		return *this;
	}
};

class VKSemaphore : public VkSyncObj<VkSemaphore>
{
public:
	VKSemaphore(VkDevice device) : VkSyncObj{ device } {}
	~VKSemaphore() noexcept;

	void Create();

public:
	VKSemaphore(const VKSemaphore&) = delete;
	VKSemaphore& operator=(const VKSemaphore&) = delete;

	VKSemaphore(VKSemaphore&& other) noexcept : VkSyncObj{ std::move(other) } {}
	VKSemaphore& operator=(VKSemaphore&& other) noexcept
	{
		VkSyncObj::operator=(std::move(other));

		return *this;
	}
};

class VKFence : public VkSyncObj<VkFence>
{
public:
	VKFence(VkDevice device) : VkSyncObj{ device } {}
	~VKFence() noexcept;

	void Create(bool signaled);

	void Wait() const;
	void Reset() const;

public:
	VKFence(const VKFence&) = delete;
	VKFence& operator=(const VKFence&) = delete;

	VKFence(VKFence&& other) noexcept : VkSyncObj{ std::move(other) } {}
	VKFence& operator=(VKFence&& other) noexcept
	{
		VkSyncObj::operator=(std::move(other));

		return *this;
	}
};

template<typename SyncType>
class VkMultiSyncObj
{
public:
	VkMultiSyncObj(VkDevice device) : m_device{ device }, m_syncObjs{}, m_currentIndex{ 0u } {}
	virtual ~VkMultiSyncObj() = default;

	void AdvanceInQueue() noexcept
	{
		++m_currentIndex;
		m_currentIndex %= std::size(m_syncObjs);
	}

	[[nodiscard]]
	auto GetFront() const noexcept { return m_syncObjs.at(m_currentIndex).Get(); }

protected:
	VkDevice              m_device;
	std::vector<SyncType> m_syncObjs;
	size_t                m_currentIndex;

public:
	VkMultiSyncObj(const VkMultiSyncObj&) = delete;
	VkMultiSyncObj& operator=(const VkMultiSyncObj&) = delete;

	VkMultiSyncObj(VkMultiSyncObj&& other) noexcept
		: m_device{ other.m_device }, m_syncObjs{ std::move(other.m_syncObjs) }
		, m_currentIndex{ other.m_currentIndex } {}

	VkMultiSyncObj& operator=(VkMultiSyncObj&& other) noexcept
	{
		m_device       = other.m_device;
		m_syncObjs     = std::move(other.m_syncObjs);
		m_currentIndex = other.m_currentIndex;

		return *this;
	}
};

class VkMultiSemaphore : public VkMultiSyncObj<VKSemaphore>
{
public:
	VkMultiSemaphore(VkDevice device, size_t semaphoreCount);

public:
	VkMultiSemaphore(const VkMultiSemaphore&) = delete;
	VkMultiSemaphore& operator=(const VkMultiSemaphore&) = delete;

	VkMultiSemaphore(VkMultiSemaphore&& other) noexcept : VkMultiSyncObj{ std::move(other) } {}
	VkMultiSemaphore& operator=(VkMultiSemaphore&& other) noexcept
	{
		VkMultiSyncObj::operator=(std::move(other));

		return *this;
	}
};

class VkMultiFence : public VkMultiSyncObj<VKFence>
{
public:
	VkMultiFence(VkDevice device, size_t fenceCount, bool signaled);

	void WaitForFrontFence() const noexcept;
	void ResetFrontFence() const noexcept;

public:
	VkMultiFence(const VkMultiFence&) = delete;
	VkMultiFence& operator=(const VkMultiFence&) = delete;

	VkMultiFence(VkMultiFence&& other) noexcept : VkMultiSyncObj{ std::move(other) } {}
	VkMultiFence& operator=(VkMultiFence&& other) noexcept
	{
		VkMultiSyncObj::operator=(std::move(other));

		return *this;
	}
};

class VkSyncObjects
{
public:
	VkSyncObjects(VkDevice device, std::uint32_t bufferCount = 1u, bool signaledFence = false)
		: m_fences{ device, bufferCount, signaledFence }, m_semaphores{ device, bufferCount } {}

	void ResetFrontFence() const noexcept { m_fences.ResetFrontFence(); }
	void WaitForFrontFence() const noexcept { m_fences.WaitForFrontFence(); }
	void AdvanceFenceInQueue() noexcept { m_fences.AdvanceInQueue(); }
	void AdvanceSemaphoreInQueue() noexcept { m_semaphores.AdvanceInQueue(); }
	void AdvanceSyncObjectsInQueue() noexcept
	{
		AdvanceFenceInQueue();
		AdvanceSemaphoreInQueue();
	}

	[[nodiscard]]
	VkFence GetFrontFence() const noexcept { return m_fences.GetFront(); }
	[[nodiscard]]
	VkSemaphore GetFrontSemaphore() const noexcept { return m_semaphores.GetFront(); }

private:
	VkMultiFence     m_fences;
	VkMultiSemaphore m_semaphores;

public:
	VkSyncObjects(const VkSyncObjects&) = delete;
	VkSyncObjects& operator=(const VkSyncObjects&) = delete;

	VkSyncObjects(VkSyncObjects&& other) noexcept
		: m_fences{ std::move(other.m_fences) }, m_semaphores{ std::move(other.m_semaphores) } {}
	VkSyncObjects& operator=(VkSyncObjects&& other) noexcept
	{
		m_fences     = std::move(other.m_fences);
		m_semaphores = std::move(other.m_semaphores);

		return *this;
	}
};
#endif
