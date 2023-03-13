#ifndef UPLOAD_CONTAINER_HPP_
#define UPLOAD_CONTAINER_HPP_
#include <memory>
#include <vector>
#include <atomic>

class UploadContainer {
public:
	UploadContainer() noexcept;

	void SetMemoryStart(std::uint8_t* dstMemoryStart) noexcept;
	void AddMemory(void const* src, size_t memorySize, size_t offset) noexcept;
	void CopyData(std::atomic_size_t& workCount) noexcept;

private:
	struct MemoryInfo {
		size_t size;
		size_t offset;
		void const* src;
	};

private:
	void CopyMem(const MemoryInfo& memInfo) const noexcept;

private:
	std::vector<MemoryInfo> m_memoryRefInfos;
	std::uint8_t* m_dstMemoryStart;
};
#endif
