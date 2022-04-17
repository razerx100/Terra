#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>

class TextureStorage {
public:
	TextureStorage(const std::vector<std::uint32_t>& queueFamilyIndices) noexcept;

	size_t AddTexture(
		VkDevice device,
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
	) noexcept;

	void CopyData(std::atomic_size_t& workCount) noexcept;
	void ReleaseUploadBuffer() noexcept;
	void CreateBuffers(VkDevice device);

private:
	VkImageCreateInfo m_createInfo;
};
#endif
