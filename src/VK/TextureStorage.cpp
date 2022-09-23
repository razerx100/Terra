#include <TextureStorage.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

TextureStorage::TextureStorage(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	std::vector<std::uint32_t> queueFamilyIndices
) : m_queueFamilyIndices{std::move(queueFamilyIndices)},
	m_textureSampler{ VK_NULL_HANDLE }, m_deviceRef{ logicalDevice } {

	CreateSampler(logicalDevice, physicalDevice, &m_textureSampler, true);
}

TextureStorage::~TextureStorage() noexcept {
	vkDestroySampler(m_deviceRef, m_textureSampler, nullptr);
}

size_t TextureStorage::AddTexture(
	VkDevice device,
	std::unique_ptr<std::uint8_t> textureDataHandle, size_t width, size_t height
) {
	static VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

	VkUploadableImageResourceView texture{ device };
	texture.CreateResource(
		device, static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height),
		imageFormat, m_queueFamilyIndices
	);

	texture.SetMemoryOffsetAndType(device);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(textureDataHandle), width * height * 4u,
		texture.GetUploadMemoryOffset()
	);

	m_textures.emplace_back(std::move(texture));

	return std::size(m_textures) - 1u;
}

void TextureStorage::ReleaseUploadBuffers() noexcept {
	for (auto& texture : m_textures)
		texture.CleanUpUploadResource();
}

void TextureStorage::BindMemories(VkDevice device) {
	for (size_t index = 0u; index < std::size(m_textures); ++index) {
		auto& texture = m_textures[index];
		texture.BindResourceToMemory(device);
		texture.CreateImageView(device, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void TextureStorage::SetDescriptorLayouts() const noexcept {
	DescriptorInfo descInfo = {};
	descInfo.bindingSlot = 1u;
	descInfo.descriptorCount = static_cast<std::uint32_t>(std::size(m_textures));
	descInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	std::vector<VkDescriptorImageInfo> imageInfos;

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.sampler = m_textureSampler;

	for (auto& texture : m_textures) {
		imageInfo.imageView = texture.GetImageView();

		imageInfos.emplace_back(imageInfo);
	}

	Terra::descriptorSet->AddSetLayout(
		descInfo, VK_SHADER_STAGE_FRAGMENT_BIT, std::move(imageInfos)
	);
}

void TextureStorage::RecordUploads(VkCommandBuffer copyCmdBuffer) noexcept {
	for (size_t index = 0u; index < std::size(m_textures); ++index)
		m_textures[index].RecordCopy(copyCmdBuffer);
}

void TextureStorage::TransitionImages(VkCommandBuffer graphicsBuffer) noexcept {
	for (auto& texture : m_textures)
		texture.TransitionImageLayout(graphicsBuffer);
}
