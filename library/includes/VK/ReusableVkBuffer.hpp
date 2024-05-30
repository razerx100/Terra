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
	size_t Add(U&& element) noexcept
	{
		auto oElementIndex = m_elements.GetFirstAvailableIndex();

		if (oElementIndex)
		{
			const size_t elementIndex = oElementIndex.value();

			m_elements.UpdateElement(elementIndex, std::forward<U>(element));

			return elementIndex;
		}
		else
		{
			const size_t elementIndex = GetCount();

			m_elements.AddNewElement(std::forward<U>(element));

			const size_t newElementCount = GetCount()
				+ static_cast<Derived*>(this)->GetExtraElementAllocationCount();

			m_elements.ReserveNewElements(newElementCount);

			static_cast<Derived*>(this)->CreateBuffer(newElementCount);

			return elementIndex;
		}
	}

	[[nodiscard]]
	// Returns the indices of the elements in the ElementBuffer.
	std::vector<size_t> AddMultiple(std::vector<T>&& elements) noexcept
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
#endif
