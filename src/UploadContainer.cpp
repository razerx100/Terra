#include <UploadContainer.hpp>
#include <cstring>

#include <Terra.hpp>

UploadContainer::UploadContainer() noexcept : m_memoryStart{ nullptr } {}

void UploadContainer::SetMemoryStart(std::uint8_t* memoryStart) noexcept {
	m_memoryStart = memoryStart;
}

void UploadContainer::AddMemory(
	std::unique_ptr<std::uint8_t> data, size_t memorySize, size_t offset
) noexcept {
	m_memoryInfos.emplace_back(memorySize, offset);
	m_memories.emplace_back(std::move(data));
}

void UploadContainer::AddMemory(void* memoryRef, size_t memorySize, size_t offset) noexcept {
	m_memoryRefInfos.emplace_back(memorySize, offset);
	m_memoryRefs.emplace_back(memoryRef);
}

void UploadContainer::CopyData(std::atomic_size_t& workCount) noexcept {
	++workCount;

	Terra::threadPool->SubmitWork(
		[&] {
			for (size_t index = 0u; index < std::size(m_memories); ++index) {
				const MemoryInfo& info = m_memoryInfos[index];

				memcpy(m_memoryStart + info.offset, m_memories[index].get(), info.size);
			}

			for (size_t index = 0u; index < std::size(m_memoryRefs); ++index) {
				const MemoryInfo& info = m_memoryRefInfos[index];

				memcpy(m_memoryStart + info.offset, m_memoryRefs[index], info.size);
			}

			--workCount;
		}
	);
}
