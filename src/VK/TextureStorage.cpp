#include <TextureStorage.hpp>
#include <VKThrowMacros.hpp>
#include <VkHelperFunctions.hpp>

#include <Terra.hpp>

TextureStorage::TextureStorage(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	std::vector<std::uint32_t> queueFamilyIndices
) : m_currentOffset(0u),
m_queueFamilyIndices(std::move(queueFamilyIndices)),
m_textureSampler(VK_NULL_HANDLE), m_deviceRef(logicalDevice) {

	m_uploadBuffers = std::make_unique<UploadBuffers>(logicalDevice, physicalDevice);

	ImageBuffer image = ImageBuffer(logicalDevice);

	image.CreateImage(
		logicalDevice,
		1u, 1u, VK_FORMAT_R8G8B8A8_SRGB,
		m_queueFamilyIndices
	);

	VkMemoryRequirements memoryReq = {};
	vkGetImageMemoryRequirements(logicalDevice, image.GetImage(), &memoryReq);

	m_textureMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, memoryReq, false
		);

	CreateSampler(logicalDevice, physicalDevice, &m_textureSampler, true);
}

TextureStorage::~TextureStorage() noexcept {
	vkDestroySampler(m_deviceRef, m_textureSampler, nullptr);
}

size_t TextureStorage::AddTexture(
	VkDevice device,
	std::unique_ptr<std::uint8_t> textureDataHandle,
	size_t width, size_t height, size_t pixelSizeInBytes
) noexcept {
	VkFormat imageFormat = VK_FORMAT_UNDEFINED;

	if (pixelSizeInBytes == 4u)
		imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	else if (pixelSizeInBytes == 16u)
		imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

	std::uint32_t width32 = static_cast<std::uint32_t>(width);
	std::uint32_t height32 = static_cast<std::uint32_t>(height);

	std::unique_ptr<ImageBuffer> imageBuffer = std::make_unique<ImageBuffer>(device);
	imageBuffer->CreateImage(
		device, width32, height32, imageFormat, m_queueFamilyIndices
	);

	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(device, imageBuffer->GetImage(), &memoryRequirements);

	size_t bufferSize = memoryRequirements.size;
	m_currentOffset = Align(m_currentOffset, memoryRequirements.alignment);

	m_uploadBuffers->AddBuffer(device, std::move(textureDataHandle), bufferSize);

	m_textureData.emplace_back(
		width32, height32, m_currentOffset, imageFormat
	);

	m_currentOffset += bufferSize;

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

void TextureStorage::CreateBuffers(VkDevice device) {
	m_textureMemory->AllocateMemory(m_currentOffset);

	m_uploadBuffers->CreateBuffers(device);

	VkDeviceMemory textureMemory = m_textureMemory->GetMemoryHandle();

	for (size_t index = 0u; index < std::size(m_textures); ++index) {
		auto& texture = m_textures[index];
		ImageData& imageData = m_textureData[index];

		texture->BindImageToMemory(
			device, textureMemory,
			static_cast<VkDeviceSize>(imageData.offset)
		);
		texture->CreateImageView(device, imageData.format);
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
		imageInfo.imageView = texture->GetImageView();

		imageInfos.emplace_back(imageInfo);
	}

	Terra::descriptorSet->AddSetLayoutAndQueueForBinding(
		descInfo, VK_SHADER_STAGE_FRAGMENT_BIT,
		std::move(imageInfos)
	);
}

void TextureStorage::RecordUploads(VkDevice device, VkCommandBuffer copyCmdBuffer) noexcept {
	m_uploadBuffers->FlushMemory(device);

	const auto& uploadBuffers = m_uploadBuffers->GetUploadBuffers();

	for (size_t index = 0u; index < std::size(m_textureData); ++index)
		m_textures[index]->CopyToImage(
			copyCmdBuffer, uploadBuffers[index]->GetBuffer(),
			m_textureData[index].width, m_textureData[index].height
		);
}

void TextureStorage::TransitionImages(VkCommandBuffer graphicsBuffer) noexcept {
	for (auto& texture : m_textures)
		texture->TransitionImageLayout(graphicsBuffer, true);
}
