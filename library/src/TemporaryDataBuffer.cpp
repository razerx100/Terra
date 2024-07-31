#include <TemporaryDataBuffer.hpp>

void TemporaryDataBufferGPU::SetUsed(size_t frameIndex) noexcept
{
	for (auto& tempBuffer : m_tempBuffers)
		if (tempBuffer.ownerIndex != std::numeric_limits<size_t>::max())
			tempBuffer.ownerIndex = frameIndex;
}

void TemporaryDataBufferGPU::Clear(size_t frameIndex) noexcept
{
	std::erase_if(
		m_tempBuffers,
		[frameIndex](const TempBuffer& tempBuffer)
		{ return tempBuffer.ownerIndex == frameIndex; }
	);
}
