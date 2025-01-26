#ifndef VK_EXTERNAL_BUFFER_HPP_
#define VK_EXTERNAL_BUFFER_HPP_
#include <ExternalBuffer.hpp>
#include <VkResources.hpp>

class VkExternalBuffer : public ExternalBuffer
{
public:
	VkExternalBuffer(
		VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType,
		VkBufferUsageFlags usageFlags
	);

	void Create(size_t bufferSize) override;

	void Destroy() noexcept override { m_buffer.Destroy(); }

	[[nodiscard]]
	size_t BufferSize() const noexcept override { return m_buffer.BufferSize(); }

	[[nodiscard]]
	std::uint8_t* CPUHandle() const override { return m_buffer.CPUHandle(); }

	[[nodiscard]]
	const Buffer& GetBuffer() const noexcept { return m_buffer; }

private:
	Buffer             m_buffer;
	VkBufferUsageFlags m_usageFlags;

public:
	VkExternalBuffer(const VkExternalBuffer&) = delete;
	VkExternalBuffer& operator=(const VkExternalBuffer&) = delete;

	VkExternalBuffer(VkExternalBuffer&& other) noexcept
		: m_buffer{ std::move(other.m_buffer) }, m_usageFlags{ other.m_usageFlags }
	{}
	VkExternalBuffer& operator=(VkExternalBuffer&& other) noexcept
	{
		m_buffer     = std::move(other.m_buffer);
		m_usageFlags = other.m_usageFlags;

		return *this;
	}
};
#endif
