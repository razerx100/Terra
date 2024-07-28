#ifndef TEMPORARY_DATA_BUFFER_HPP_
#define TEMPORARY_DATA_BUFFER_HPP_
#include <vector>
#include <memory>

class TemporaryDataBuffer
{
public:
	TemporaryDataBuffer() : m_tempBuffer{}, m_isUsed{ false } {};

	void Add(std::shared_ptr<void> tempData) noexcept
	{
		m_tempBuffer.emplace_back(std::move(tempData));
	}

	void SetUsed() noexcept { m_isUsed = true; }

	void Clear() noexcept
	{
		if (m_isUsed)
		{
			m_tempBuffer.clear();
			m_isUsed = false;
		}
	}

private:
	std::vector<std::shared_ptr<void>> m_tempBuffer;
	bool                               m_isUsed;

public:
	TemporaryDataBuffer(const TemporaryDataBuffer&) = delete;
	TemporaryDataBuffer& operator=(const TemporaryDataBuffer&) = delete;

	TemporaryDataBuffer(TemporaryDataBuffer&& other) noexcept
		: m_tempBuffer{ std::move(other.m_tempBuffer) },
		m_isUsed{ other.m_isUsed }
	{}
	TemporaryDataBuffer& operator=(TemporaryDataBuffer&& other) noexcept
	{
		m_tempBuffer = std::move(other.m_tempBuffer);
		m_isUsed     = other.m_isUsed;

		return *this;
	}
};
#endif
