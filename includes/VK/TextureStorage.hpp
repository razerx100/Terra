#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <VkResourceViews.hpp>
#include <VkHelperFunctions.hpp>
#include <optional>

class TextureStorage {
public:
	struct Args {
		std::optional<VkDevice> logicalDevice;
		std::optional<VkPhysicalDevice> physicalDevice;
		std::optional<QueueIndicesTG> queueIndices;
	};

public:
	TextureStorage(const Args& arguments);
	~TextureStorage() noexcept;

	size_t AddTexture(
		VkDevice device,
		std::unique_ptr<std::uint8_t> textureDataHandle, size_t width, size_t height
	);

	void RecordUploads(VkCommandBuffer transferCmdBuffer) noexcept;
	void AcquireOwnerShips(VkCommandBuffer graphicsCmdBuffer) noexcept;
	void ReleaseOwnerships(VkCommandBuffer transferCmdBuffer) noexcept;
	void TransitionImages(VkCommandBuffer graphicsCmdBuffer) noexcept;
	void SetDescriptorLayouts() const noexcept;

	void ReleaseUploadBuffers() noexcept;
	void BindMemories(VkDevice device);

private:
	std::vector<VkUploadableImageResourceView> m_textures;
	VkSampler m_textureSampler;
	VkDevice m_deviceRef;
	QueueIndicesTG m_queueIndices;
};
#endif
