#include <UploadContainer.hpp>
#include <cstring>

#include <Terra.hpp>

UploadContainer::UploadContainer() noexcept : m_dstMemoryStart{ nullptr } {}

void UploadContainer::SetMemoryStart(std::uint8_t* dstMemoryStart) noexcept {
	m_dstMemoryStart = dstMemoryStart;
}

void UploadContainer::AddMemory(
	void const* src, size_t memorySize, size_t offset
) noexcept {
	MemoryInfo memInfo{
		.size = memorySize,
		.offset = offset,
		.src = src
	};

	m_memoryRefInfos.emplace_back(memInfo);
}

void UploadContainer::CopyData(std::atomic_size_t& workCount) noexcept {
	++workCount;

	Terra::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_memoryRefInfos); ++index)
				CopyMem(m_memoryRefInfos[index]);

			--workCount;
		}
	);
}

void UploadContainer::CopyMem(const MemoryInfo& memInfo) const noexcept {
	memcpy(m_dstMemoryStart + memInfo.offset, memInfo.src, memInfo.size);
}
