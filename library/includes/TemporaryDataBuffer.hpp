#ifndef TEMPORARY_DATA_BUFFER_HPP_
#define TEMPORARY_DATA_BUFFER_HPP_
#include <vector>
#include <memory>

class TemporaryDataBuffer
{
public:
	TemporaryDataBuffer() = default;

	template<typename T>
	void Add(std::shared_ptr<T> tempData) noexcept
	{
		m_tempBuffer.emplace_back(std::move(tempData));
	}

	void Clear() noexcept { m_tempBuffer.clear(); }

private:
	std::vector<std::shared_ptr<void>> m_tempBuffer;

public:
	TemporaryDataBuffer(const TemporaryDataBuffer&) = delete;
	TemporaryDataBuffer& operator=(const TemporaryDataBuffer&) = delete;

	TemporaryDataBuffer(TemporaryDataBuffer&& other) noexcept
		: m_tempBuffer{ std::move(other.m_tempBuffer) }
	{}
	TemporaryDataBuffer& operator=(TemporaryDataBuffer&& other) noexcept
	{
		m_tempBuffer = std::move(other.m_tempBuffer);

		return *this;
	}
};
#endif
