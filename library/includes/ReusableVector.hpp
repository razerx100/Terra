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

	template<typename U>
	// It has a different template, cuz the ctor might be able to receive a different value.
	void AddNewElement(U&& element) noexcept
	{
		m_elements.emplace_back(std::forward<U>(element));
		m_availableIndices.emplace_back(false);
	}
	template<typename U>
	// It has a different template, cuz the ctor might be able to receive a different value.
	void UpdateElement(size_t elementIndex, U&& element) noexcept
	{
		m_elements.at(elementIndex)         = std::forward<U>(element);
		m_availableIndices.at(elementIndex) = false;
	}
	void ReserveNewElements(size_t newCount) noexcept
	{
		// Can't use resize for the elements container, as it creates empty items. For example,
		// if we add a single model shared_ptr and the allocationCount is 4. 4 empty shared_ptrs
		// would be added and only a single one will be populated. But we will be able to access
		// the three empty ones as well, and the program can crash trying to access those.
		m_elements.reserve(newCount);
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
	size_t Add(U&& element, size_t extraAllocCount) noexcept
	{
		size_t elementIndex = GetNextFreeIndex(extraAllocCount);

		// The boolean available indices container represents the possible allocation count. The
		// element count should always be less than or equal to the index. Or it would mean that
		// there are non-removed null elements. Which should have probably caused a crash by now.
		// But lets still do an assertion.
		// And the available indices' size will always be bigger.
		assert(
			std::size(m_elements) <= elementIndex &&
			"The index is less than the element count. Meaning there are null elements"
		);

		m_elements.emplace_back(std::move(element));
		m_availableIndices.at(elementIndex) = false;

		return elementIndex;
	}

	template<typename U>
	size_t Add(U&& element) noexcept
	{
		return Add(std::move(element), 0u);
	}

	void RemoveElement(size_t index) noexcept
	{
		// We want to avoid reallocation of the vector as much as possible. But erase shouldn't
		// change the capacity. And also we don't want a null object lingering around which can
		// be accessed.
		m_elements.erase(std::next(std::begin(m_elements), index));
		m_availableIndices.at(index) = true;
	}

	[[nodiscard]]
	std::optional<size_t> GetFirstAvailableIndex() const noexcept
	{
		return ::GetFirstAvailableIndex(m_availableIndices);
	}
	[[nodiscard]]
	std::vector<size_t> GetAvailableIndices() const noexcept
	{
		return ::GetAvailableIndices(m_availableIndices);
	}

	T& at(size_t index) noexcept { return m_elements.at(index); }
	const T& at(size_t index) const noexcept { return m_elements.at(index); }

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
	[[nodiscard]]
	// I cannot be sure that the initial capacity of the elements vector will be 0, it should though.
	// Lets just use the size of the availableIndices just in case. The capacity of elements and
	// size of availableIndices should be the same.
	size_t GetCapacityCount() const noexcept { return std::size(m_availableIndices); }

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
