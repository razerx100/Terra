#include <TextureStorage.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

TextureStorage::TextureStorage(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	std::vector<std::uint32_t> queueFamilyIndices
) : m_queueFamilyIndices{std::move(queueFamilyIndices)},
	m_textureSampler{ VK_NULL_HANDLE }, m_deviceRef{ logicalDevice } {

	m_uploadBuffers = std::make_unique<UploadBuffers>();

	CreateSampler(logicalDevice, physicalDevice, &m_textureSampler, true);
}

TextureStorage::~TextureStorage() noexcept {
	vkDestroySampler(m_deviceRef, m_textureSampler, nullptr);
}

size_t TextureStorage::AddTexture(
	VkDevice device,
	std::unique_ptr<std::uint8_t> textureDataHandle, size_t width, size_t height
) {
	VkFormat imageFormat = VK_FORMAT_UNDEFINED;

	size_t bytesPerPixel = 4u;

	imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

	auto width32 = static_cast<std::uint32_t>(width);
	auto height32 = static_cast<std::uint32_t>(height);

	size_t bufferSize = width * height * bytesPerPixel;
	m_uploadBuffers->AddBuffer(device, std::move(textureDataHandle), bufferSize);

	VkImageResourceView imageBuffer{ device };
	imageBuffer.CreateResource(
		device, width32, height32, imageFormat,
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		m_queueFamilyIndices
	);

	VkMemoryRequirements memoryRequirements{};
	vkGetImageMemoryRequirements(device, imageBuffer.GetResource(), &memoryRequirements);

	if (!Terra::Resources::gpuOnlyMemory->CheckMemoryType(memoryRequirements))
		VK_GENERIC_THROW("Memory Type doesn't match with Image Buffer requirements.");

	const VkDeviceSize textureOffset =
		Terra::Resources::gpuOnlyMemory->ReserveSizeAndGetOffset(memoryRequirements);

	m_textureOffsets.emplace_back(textureOffset);

	m_textures.emplace_back(std::move(imageBuffer));

	return std::size(m_textures) - 1u;
}

void TextureStorage::CopyData(std::atomic_size_t& workCount) noexcept {
	workCount += 1;

	Terra::threadPool->SubmitWork(
		[&workCount, &uploadBuffers = m_uploadBuffers] {
			uploadBuffers->CopyData();

			--workCount;
		}
	);
}

void TextureStorage::ReleaseUploadBuffers() noexcept {
	m_uploadBuffers.reset();
}

void TextureStorage::BindMemories(VkDevice device) {
	m_uploadBuffers->BindMemories(device);

	VkDeviceMemory textureMemory = Terra::Resources::gpuOnlyMemory->GetMemoryHandle();

	for (size_t index = 0u; index < std::size(m_textures); ++index) {
		auto& texture = m_textures[index];
		texture.BindResourceToMemory(device, textureMemory, m_textureOffsets[index]);
		texture.CreateImageView(device, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void TextureStorage::SetDescriptorLayouts() const noexcept {
	DescriptorInfo descInfo = {};
	descInfo.bindingSlot = 1u;
	descInfo.descriptorCount = static_cast<std::uint32_t>(std::size(m_textures));
	descInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	std::vector<VkDescriptorImageInfo> imageInfos;

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.sampler = m_textureSampler;

	for (auto& texture : m_textures) {
		imageInfo.imageView = texture.GetImageView();

		imageInfos.emplace_back(imageInfo);
	}

	Terra::descriptorSet->AddSetLayoutAndQueueForBinding(
		descInfo, VK_SHADER_STAGE_FRAGMENT_BIT, std::move(imageInfos)
	);
}

void TextureStorage::RecordUploads(VkCommandBuffer copyCmdBuffer) noexcept {
	const auto& uploadBuffers = m_uploadBuffers->GetUploadBuffers();

	for (size_t index = 0u; index < std::size(m_textures); ++index)
		m_textures[index].RecordImageCopy(copyCmdBuffer, uploadBuffers[index]->GetResource());
}

void TextureStorage::TransitionImages(VkCommandBuffer graphicsBuffer) noexcept {
	for (auto& texture : m_textures)
		texture.TransitionImageLayout(graphicsBuffer, true);
}
