#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <VkResourceViews.hpp>
#include <optional>

class TextureStorage {
public:
	struct Args {
		std::optional<VkDevice> logicalDevice;
		std::optional<VkPhysicalDevice> physicalDevice;
	};

public:
	TextureStorage(const Args& arguments);
	~TextureStorage() noexcept;

	size_t AddTexture(
		VkDevice device,
		std::unique_ptr<std::uint8_t> textureDataHandle, size_t width, size_t height
	);

	void RecordUploads(VkCommandBuffer copyCmdBuffer) noexcept;
	void AcquireOwnerShips(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;
	void ReleaseOwnerships(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;
	void TransitionImages(VkCommandBuffer graphicsBuffer) noexcept;
	void SetDescriptorLayouts() const noexcept;

	void ReleaseUploadBuffers() noexcept;
	void BindMemories(VkDevice device);

private:
	std::vector<VkUploadableImageResourceView> m_textures;
	VkSampler m_textureSampler;
	VkDevice m_deviceRef;
};
#endif
