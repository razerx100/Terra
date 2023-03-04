#ifndef UPLOAD_CONTAINER_HPP_
#define UPLOAD_CONTAINER_HPP_
#include <memory>
#include <vector>

class UploadContainer {
public:
	UploadContainer() noexcept;

	void SetMemoryStart(std::uint8_t* memoryStart) noexcept;
	void AddMemory(
		std::unique_ptr<std::uint8_t> data, size_t memorySize, size_t offset
	) noexcept;
	void AddMemory(void* memoryRef, size_t memorySize, size_t offset) noexcept;
	void CopyData(std::atomic_size_t& workCount) noexcept;

private:
	struct MemoryInfo {
		size_t size;
		size_t offset;
	};

private:
	std::vector<MemoryInfo> m_memoryInfos;
	std::vector<std::unique_ptr<std::uint8_t>> m_memories;
	std::vector<MemoryInfo> m_memoryRefInfos;
	std::vector<void*> m_memoryRefs;
	std::uint8_t* m_memoryStart;
};
#endif
