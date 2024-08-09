#ifndef VK_SYNC_OBJECTS_HPP_
#define VK_SYNC_OBJECTS_HPP_
#include <vulkan/vulkan.hpp>
#include <utility>

template<typename ObjType>
class VkSyncObj
{
public:
	VkSyncObj(VkDevice device) : m_device{ device }, m_syncObj{ VK_NULL_HANDLE } {}
	virtual ~VkSyncObj() = default;

	[[nodiscard]]
	ObjType Get() const noexcept { return m_syncObj; }

protected:
	virtual void SelfDestruct() noexcept = 0;

protected:
	VkDevice m_device;
	ObjType  m_syncObj;

public:
	VkSyncObj(const VkSyncObj&) = delete;
	VkSyncObj& operator=(const VkSyncObj&) = delete;

	VkSyncObj(VkSyncObj&& other) noexcept
		: m_device{ other.m_device }, m_syncObj{ std::exchange(other.m_syncObj, VK_NULL_HANDLE) }
	{}
	VkSyncObj& operator=(VkSyncObj&& other) noexcept
	{
		SelfDestruct();

		m_device  = other.m_device;
		m_syncObj = std::exchange(other.m_syncObj, VK_NULL_HANDLE);

		return *this;
	}
};

class VKSemaphore : public VkSyncObj<VkSemaphore>
{
public:
	VKSemaphore(VkDevice device) : VkSyncObj{ device } {}
	~VKSemaphore() noexcept override;

	void Create(bool timeline = false, std::uint64_t initialValue = 0u);

	void Signal(std::uint64_t signalValue = 1u) const noexcept;
	void Wait(std::uint64_t waitValue = 1u) const noexcept;

	[[nodiscard]]
	std::uint64_t GetCurrentValue() const noexcept;

private:
	void SelfDestruct() noexcept override;

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
	~VKFence() noexcept override;

	void Create(bool signaled);

	void Wait() const;
	void Reset() const;

private:
	void SelfDestruct() noexcept override;

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
#endif
