#ifndef TEXTURE_STORAGE_HPP_
#define TEXTURE_STORAGE_HPP_
#include <vulkan/vulkan.hpp>
#include <atomic>
#include <VkBuffers.hpp>
#include <UploadBuffers.hpp>
#include <DeviceMemory.hpp>

class TextureStorage {
public:
	TextureStorage(
		VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
		std::vector<std::uint32_t> queueFamilyIndices
	);
	~TextureStorage() noexcept;

	size_t AddTexture(
		VkDevice device,
		std::unique_ptr<std::uint8_t> textureDataHandle,
		size_t width, size_t height, bool components16bits = false
	) noexcept;

	void RecordUploads(VkDevice device, VkCommandBuffer copyCmdBuffer) noexcept;
	void TransitionImages(VkCommandBuffer graphicsBuffer) noexcept;
	void SetDescriptorLayouts() const noexcept;

	void CopyData(std::atomic_size_t& workCount) noexcept;
	void ReleaseUploadBuffers() noexcept;
	void CreateBuffers(VkDevice device);

private:
	struct ImageData {
		std::uint32_t width;
		std::uint32_t height;
		size_t offset;
		VkFormat format;
	};

private:
	size_t m_currentOffset;
	std::vector<std::unique_ptr<ImageBuffer>> m_textures;
	std::unique_ptr<DeviceMemory> m_textureMemory;
	std::unique_ptr<UploadBuffers> m_uploadBuffers;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	std::vector<ImageData> m_textureData;
	VkSampler m_textureSampler;
	VkDevice m_deviceRef;
};
#endif
