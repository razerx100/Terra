#include <TextureStorage.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

TextureStorage::TextureStorage(const Args& arguments)
	: m_textureSampler{ VK_NULL_HANDLE }, m_deviceRef{ arguments.logicalDevice.value() },
	m_queueIndices{ arguments.queueIndices.value() } {

	CreateSampler(
		arguments.logicalDevice.value(), arguments.physicalDevice.value(), &m_textureSampler,
		true
	);
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
		imageFormat
	);

	texture.SetMemoryOffsetAndType(device);

	Terra::Resources::uploadContainer->AddMemory(
		std::move(textureDataHandle), width * height * 4u, texture.GetFirstUploadMemoryOffset()
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
	DescriptorInfo descInfo{};
	descInfo.bindingSlot = 2u;
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

	Terra::graphicsDescriptorSet->AddImagesContiguous(
		descInfo, std::move(imageInfos), VK_SHADER_STAGE_FRAGMENT_BIT
	);
}

void TextureStorage::RecordUploads(VkCommandBuffer transferCmdBuffer) noexcept {
	for (auto& texture : m_textures)
		texture.RecordCopy(transferCmdBuffer);
}

void TextureStorage::TransitionImages(VkCommandBuffer graphicsCmdBuffer) noexcept {
	for (auto& texture : m_textures)
		texture.TransitionImageLayout(graphicsCmdBuffer);
}

void TextureStorage::AcquireOwnerShips(VkCommandBuffer graphicsCmdBuffer) noexcept {
	for (auto& texture : m_textures)
		texture.AcquireOwnership(
			graphicsCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics,
			VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
		);
}

void TextureStorage::ReleaseOwnerships(VkCommandBuffer transferCmdBuffer) noexcept {
	for (auto& texture : m_textures)
		texture.ReleaseOwnerShip(
			transferCmdBuffer, m_queueIndices.transfer, m_queueIndices.graphics
		);
}
