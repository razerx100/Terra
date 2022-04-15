#ifndef __TEXTURE_STORAGE_HPP__
#define __TEXTURE_STORAGE_HPP__
#include <ITextureStorage.hpp>

class TextureStorage : public ITextureStorage {
public:
	TextureStorage(const std::vector<std::uint32_t>& queueFamilyIndices) noexcept;

	size_t AddTexture(
		VkDevice device,
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
	) noexcept override;

	void CopyData(std::atomic_size_t& workCount) noexcept override;
	void ReleaseUploadBuffer() noexcept override;
	void CreateBuffers(VkDevice device) override;

private:
	VkImageCreateInfo m_createInfo;
};
#endif
