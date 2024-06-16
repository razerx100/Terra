#ifndef REUSABLE_VK_BUFFER_HPP_
#define REUSABLE_VK_BUFFER_HPP_
#include <ReusableVector.hpp>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>
#include <utility>
#include <concepts>

template<class Derived, class T>
class ReusableVkBuffer
{
public:
	ReusableVkBuffer(
		VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType
	) : m_buffers{ device, memoryManager, memoryType }, m_elements{}
	{}

	template<typename U>
	[[nodiscard]]
	// Returns the index of the element in the ElementBuffer.
	size_t Add(U&& element)
	{
		const size_t oldCount          = m_elements.GetCount();
		const size_t extraElementCount = static_cast<Derived*>(this)->GetExtraElementAllocationCount();

		const size_t elementIndex      = m_elements.Add(std::forward<U>(element), extraElementCount);

		const size_t newCount          = m_elements.GetCount();

		if(newCount > oldCount)
			static_cast<Derived*>(this)->CreateBuffer(newCount);

		return elementIndex;
	}

	[[nodiscard]]
	// Returns the indices of the elements in the ElementBuffer.
	std::vector<size_t> AddMultiple(std::vector<T>&& elements)
	{
		std::vector<size_t> freeIndices = m_elements.GetAvailableIndices();

		for (size_t index = 0u; index < std::size(freeIndices); ++index)
		{
			const size_t freeIndex = freeIndices.at(index);

			m_elements.UpdateElement(freeIndex, std::move(elements.at(index)));
		}

		const size_t newElementCount = std::size(elements);
		const size_t freeIndexCount  = std::size(freeIndices);

		if (newElementCount > freeIndexCount)
		{
			// Since the free indices might not be in order. But the last one should have the
			// highest index.
			const size_t newFreeIndex = std::empty(freeIndices) ? freeIndexCount : freeIndices.back() + 1u;

			for (size_t index = freeIndexCount, freeIndex = newFreeIndex;
				index < newElementCount; ++index, ++freeIndex)
			{
				m_elements.AddNewElement(std::move(elements.at(index)));

				freeIndices.emplace_back(freeIndex);
			}

			const size_t newExtraElementCount = newElementCount
				+ static_cast<Derived*>(this)->GetExtraElementAllocationCount();

			m_elements.ReserveNewElements(newExtraElementCount);

			static_cast<Derived*>(this)->CreateBuffer(newExtraElementCount);
		}

		// Now these are the new used indices.
		freeIndices.resize(newElementCount);

		return freeIndices;
	}

	void Remove(size_t index) noexcept
	{
		static_cast<Derived*>(this)->_remove(index);
	}

protected:
	[[nodiscard]]
	size_t GetCount() const noexcept { return m_elements.GetCount(); }

protected:
	Buffer            m_buffers;
	ReusableVector<T> m_elements;

public:
	ReusableVkBuffer(const ReusableVkBuffer&) = delete;
	ReusableVkBuffer& operator=(const ReusableVkBuffer&) = delete;

	ReusableVkBuffer(ReusableVkBuffer&& other) noexcept
		: m_buffers{ std::move(other.m_buffers) }, m_elements{ std::move(other.m_elements) }
	{}
	ReusableVkBuffer& operator=(ReusableVkBuffer&& other) noexcept
	{
		m_buffers  = std::move(other.m_buffers);
		m_elements = std::move(other.m_elements);

		return *this;
	}
};

template<typename T>
class ReusableCPUBuffer
{
private:
	[[nodiscard]]
	static consteval size_t GetStride() noexcept { return sizeof(T); }
	[[nodiscard]]
	// Chose 4 for not particular reason.
	static consteval size_t GetExtraElementAllocationCount() noexcept { return 4u; }

	void CreateBuffer(size_t elementCount)
	{
		constexpr size_t strideSize = GetStride();
		const auto buffersSize      = static_cast<VkDeviceSize>(strideSize * elementCount);

		Buffer newBuffer = GetCPUResource<Buffer>(m_device, m_memoryManager);
		newBuffer.Create(buffersSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, {});

		const VkDeviceSize oldBufferSize = m_buffer.Size();
		if (oldBufferSize)
			memcpy(newBuffer.CPUHandle(), m_buffer.CPUHandle(), m_buffer.Size());

		m_buffer = std::move(newBuffer);
	}

	void CreateBufferIfNecessary(size_t index)
	{
		const VkDeviceSize currentSize = m_buffer.Size();
		constexpr size_t strideSize    = GetStride();

		const auto minimumSpaceRequirement = static_cast<VkDeviceSize>(index * strideSize + strideSize);

		if (currentSize < minimumSpaceRequirement)
			CreateBuffer(index + 1u + GetExtraElementAllocationCount());
	}

public:
	ReusableCPUBuffer(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_buffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT }
	{}

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t bindingSlot
	) const {
		descriptorBuffer.SetStorageBufferDescriptor(m_buffer, bindingSlot, 0u);
	}

	void Add(size_t index, const T& value)
	{
		CreateBufferIfNecessary(index);

		std::uint8_t* bufferOffsetPtr = m_buffer.CPUHandle();
		constexpr size_t strideSize   = GetStride();
		const size_t bufferOffset     = index * strideSize;

		memcpy(bufferOffsetPtr + bufferOffset, &value, strideSize);
	}

private:
	VkDevice       m_device;
	MemoryManager* m_memoryManager;
	Buffer         m_buffer;

public:
	ReusableCPUBuffer(const ReusableCPUBuffer&) = delete;
	ReusableCPUBuffer& operator=(const ReusableCPUBuffer&) = delete;

	ReusableCPUBuffer(ReusableCPUBuffer&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_buffer{ std::move(other.m_buffer) }
	{}
	ReusableCPUBuffer& operator=(ReusableCPUBuffer&& other) noexcept
	{
		m_device        = other.m_device;
		m_memoryManager = other.m_memoryManager;
		m_buffer        = std::move(other.m_buffer);

		return *this;
	}
};
#endif
