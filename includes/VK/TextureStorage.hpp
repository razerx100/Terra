#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <VkResourceViews.hpp>
#include <UploadBuffers.hpp>

class TextureStorage {
public:
	TextureStorage(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		std::vector<std::uint32_t> queueFamilyIndices
	);
	~TextureStorage() noexcept;

	size_t AddTexture(
		VkDevice device,
		std::unique_ptr<std::uint8_t> textureDataHandle, size_t width, size_t height
	);

	void RecordUploads(VkCommandBuffer copyCmdBuffer) noexcept;
	void TransitionImages(VkCommandBuffer graphicsBuffer) noexcept;
	void SetDescriptorLayouts() const noexcept;

	void CopyData(std::atomic_size_t& workCount) noexcept;
	void ReleaseUploadBuffers() noexcept;
	void BindMemories(VkDevice device);

private:
	std::vector<VkImageResourceView> m_textures;
	std::unique_ptr<UploadBuffers> m_uploadBuffers;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	VkSampler m_textureSampler;
	VkDevice m_deviceRef;
};
#endif
