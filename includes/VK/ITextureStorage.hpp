#ifndef __I_TEXTURE_STORAGE_HPP__
#define __I_TEXTURE_STORAGE_HPP__
#include <vulkan/vulkan.hpp>
#include <atomic>

class ITextureStorage {
public:
	virtual ~ITextureStorage() = default;

	virtual size_t AddTexture(
		VkDevice device,
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
	) noexcept = 0;

	virtual void CopyData(std::atomic_size_t& workCount) noexcept = 0;
	virtual void ReleaseUploadBuffer() noexcept = 0;
	virtual void CreateBuffers(VkDevice device) = 0;
};
#endif
