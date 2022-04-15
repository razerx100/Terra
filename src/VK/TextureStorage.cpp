#include <TextureStorage.hpp>
#include <VKThrowMacros.hpp>

TextureStorage::TextureStorage(const std::vector<std::uint32_t>& queueFamilyIndices) noexcept
	: m_createInfo{} {

	m_createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	m_createInfo.imageType = VK_IMAGE_TYPE_2D;
	m_createInfo.mipLevels = 1u;
	m_createInfo.arrayLayers = 1u;
	m_createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	m_createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	m_createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	m_createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	if (queueFamilyIndices.size() > 1u) {
		m_createInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		m_createInfo.queueFamilyIndexCount = static_cast<std::uint32_t>(
			queueFamilyIndices.size()
			);
		m_createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}
	else
		m_createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}

size_t TextureStorage::AddTexture(
		VkDevice device,
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
) noexcept {
	if (pixelSizeInBytes == 4u)
		m_createInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
	else if (pixelSizeInBytes == 16u)
		m_createInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;

	VkExtent3D imageExtent = {};
	imageExtent.width = static_cast<std::uint32_t>(width);
	imageExtent.height = static_cast<std::uint32_t>(height);
	imageExtent.depth = 1u;

	m_createInfo.extent = imageExtent;

	VkImage image;

	VkResult result;
	//VK_THROW_FAILED(result,
	//	vkCreateImage(device, &m_createInfo, nullptr, &image)
	//);

	return 0u;
}

void TextureStorage::CopyData(std::atomic_size_t& workCount) noexcept {

}

void TextureStorage::ReleaseUploadBuffer() noexcept {
}

void TextureStorage::CreateBuffers(VkDevice device) {
}
