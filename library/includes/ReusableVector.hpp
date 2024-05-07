#ifndef REUSABLE_VECTOR_HPP_
#define REUSABLE_VECTOR_HPP_
#include <vector>
#include <optional>
#include <ranges>
#include <algorithm>
#include <type_traits>
#include <utility>

template<typename T>
class ReusableVector
{
public:
	ReusableVector() = default;
	ReusableVector(size_t initialSize)
		: m_elements{ initialSize }, m_availableIndices( initialSize )
	{}

	template<typename U>
	void AddNewElement(U&& element) noexcept
	{
		m_elements.emplace_back(std::forward<U>(element));
		m_availableIndices.emplace_back(false);
	}
	template<typename U>
	void UpdateElement(size_t elementIndex, U&& element) noexcept
	{
		m_elements.at(elementIndex)         = std::forward<U>(element);
		m_availableIndices.at(elementIndex) = false;
	}
	void ReserveNewElements(size_t newCount) noexcept
	{
		m_elements.resize(newCount);
		m_availableIndices.resize(newCount, true);
	}
	void RemoveElement(size_t index) noexcept
	{
		// If there is no clear function, then there is no need to update the freed index.
		// We can just add the new item when needed.
		m_availableIndices.at(index) = true;
	}
	void RemoveElement(size_t index, void(T::*clearFunction)()) noexcept
	{
		if constexpr (std::is_pointer_v<T>)
			(m_elements.at(index)->*clearFunction)();
		else
			(m_elements.at(index).*clearFunction)();
		m_availableIndices.at(index) = true;
	}

	[[nodiscard]]
	std::optional<size_t> GetFirstAvailableIndex() const noexcept
	{
		auto result = std::ranges::find(m_availableIndices, true);

		if (result != std::end(m_availableIndices))
			return static_cast<size_t>(std::distance(std::begin(m_availableIndices), result));
		else
			return {};
	}
	[[nodiscard]]
	std::vector<size_t> GetAvailableIndices() const noexcept
	{
		// Someday I will use a cutom CPU allocator for in the vector below.
		std::vector<size_t> freeIndices{};

		auto result = std::ranges::find(m_availableIndices, true);

		while (result != std::end(m_availableIndices))
		{
			freeIndices.emplace_back(
				static_cast<size_t>(std::distance(std::begin(m_availableIndices), result))
			);
			++result;

			result = std::find(result, std::end(m_availableIndices), true);
		}

		return freeIndices;
	}

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
