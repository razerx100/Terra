#ifndef REUSABLE_VECTOR_HPP_
#define REUSABLE_VECTOR_HPP_
#include <vector>
#include <optional>
#include <type_traits>
#include <utility>

[[nodiscard]]
std::optional<size_t> GetFirstAvailableIndex(const std::vector<bool>& availableIndices) noexcept;
[[nodiscard]]
std::vector<size_t> GetAvailableIndices(const std::vector<bool>& availableIndices) noexcept;

template<typename T>
class ReusableVector
{
public:
	ReusableVector() = default;
	ReusableVector(size_t initialSize)
		: m_elements{ initialSize }, m_availableIndices( initialSize )
	{}

	void ReserveNewElements(size_t newCount) noexcept
	{
		m_elements.resize(newCount);
		m_availableIndices.resize(newCount, true);
	}

	[[nodiscard]]
	size_t GetNextFreeIndex(size_t extraAllocCount = 0) noexcept
	{
		size_t elementIndex = std::numeric_limits<size_t>::max();

		auto oElementIndex  = GetFirstAvailableIndex();

		if (oElementIndex)
			elementIndex = oElementIndex.value();
		else
		{
			// This part should only be executed when both the availableIndices and elements
			// containers have the same size. So, getting the size should be fine.
			elementIndex                 = GetCount();

			// ElementIndex is the previous size, we have the new item, and then the extraAllocations.
			const size_t newElementCount = elementIndex + 1u + extraAllocCount;

			ReserveNewElements(newElementCount);
		}

		return elementIndex;
	}

	template<typename U>
	// To use this, T must have a copy ctor for the reserving.
	size_t Add(U&& element, size_t extraAllocCount)
	{
		size_t elementIndex = GetNextFreeIndex(extraAllocCount);

		m_elements[elementIndex]         = std::move(element);
		m_availableIndices[elementIndex] = false;

		return elementIndex;
	}

	template<typename U>
	size_t Add(U&& element)
	{
		return Add(std::move(element), 0u);
	}

	void RemoveElement(size_t index) noexcept
	{
		m_elements[index] = T{};
		ToggleAvailability(index, true);
	}

	void ToggleAvailability(size_t index, bool on) noexcept
	{
		m_availableIndices[index] = on;
	}

	bool IsElementAvailable(size_t index) const noexcept { return m_availableIndices[index]; }

	T& at(size_t index) noexcept { return m_elements[index]; }
	const T& at(size_t index) const noexcept { return m_elements[index]; }

	[[nodiscard]]
	const std::vector<T>& Get() const noexcept { return m_elements; }
	[[nodiscard]]
	std::vector<T>& Get() noexcept { return m_elements; }
	[[nodiscard]]
	T const* GetPtr() const noexcept { return std::data(m_elements); }
	[[nodiscard]]
	T* GetPtr() noexcept { return std::data(m_elements); }
	[[nodiscard]]
	size_t GetCount() const noexcept { return std::size(m_elements); }

private:
	[[nodiscard]]
	std::optional<size_t> GetFirstAvailableIndex() const noexcept
	{
		return ::GetFirstAvailableIndex(m_availableIndices);
	}

private:
	std::vector<T>    m_elements;
	std::vector<bool> m_availableIndices;

public:
	ReusableVector(const ReusableVector& other) noexcept
		: m_elements{ other.m_elements }, m_availableIndices{ other.m_availableIndices }
	{}
	ReusableVector& operator=(const ReusableVector& other) noexcept
	{
		m_elements         = other.m_elements;
		m_availableIndices = other.m_availableIndices;

		return *this;
	}
	ReusableVector(ReusableVector&& other) noexcept
		: m_elements{ std::move(other.m_elements) },
		m_availableIndices{ std::move(other.m_availableIndices) }
	{}
	ReusableVector& operator=(ReusableVector&& other) noexcept
	{
		m_elements         = std::move(other.m_elements);
		m_availableIndices = std::move(other.m_availableIndices);

		return *this;
	}
};
#endif
