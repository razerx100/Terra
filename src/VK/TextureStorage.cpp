#include <TextureStorage.hpp>
#include <VKThrowMacros.hpp>
#include <CRSMath.hpp>
#include <Terra.hpp>
#include <ObjectCreationFunctions.hpp>

TextureStorage::TextureStorage(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	std::vector<std::uint32_t> queueFamilyIndices
) : m_currentOffset(0u),
	m_queueFamilyIndices(std::move(queueFamilyIndices)),
	m_textureSampler(VK_NULL_HANDLE), m_deviceRef(logicalDevice) {

	m_uploadBuffers = std::make_unique<UploadBuffers>(
		logicalDevice, physicalDevice
		);
	m_textureMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, m_queueFamilyIndices, false, BufferType::Image
		);

	CreateSampler(logicalDevice, physicalDevice, &m_textureSampler, true);
}

TextureStorage::~TextureStorage() noexcept {
	vkDestroySampler(m_deviceRef, m_textureSampler, nullptr);
}

size_t TextureStorage::AddTexture(
		VkDevice device,
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
) noexcept {
	VkFormat imageFormat = VK_FORMAT_UNDEFINED;

	if (pixelSizeInBytes == 4u)
		imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	else if (pixelSizeInBytes == 16u)
		imageFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

	m_textureData.emplace_back(
		static_cast<std::uint32_t>(width),
		static_cast<std::uint32_t>(height),
		m_currentOffset, imageFormat
	);

	size_t bufferSize = width * height * pixelSizeInBytes;

	m_currentOffset += Ceres::Math::Align(
		bufferSize, m_textureMemory->GetAlignment()
	);

	m_uploadBuffers->AddBuffer(device, data, bufferSize);

	std::unique_ptr<ImageBuffer> imageBuffer = std::make_unique<ImageBuffer>(device);

	imageBuffer->CreateImage(
		device,
		m_textureData.back().width,
		m_textureData.back().height,
		imageFormat,
		m_queueFamilyIndices
	);

	m_textures.emplace_back(std::move(imageBuffer));

	return m_textures.size() - 1u;
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

	for (size_t index = 0u; index < m_textures.size(); ++index) {
		auto& texture = m_textures[index];
		ImageData& imageData = m_textureData[index];

		texture->BindImageToMemory(
			device, textureMemory,
			static_cast<VkDeviceSize>(imageData.offset)
		);
		texture->CreateImageView(device, imageData.format);
	}
}

void TextureStorage::SetDescriptorLayouts(VkDevice device) noexcept {
	BindImageViewInputInfo inputInfo = {};
	inputInfo.device = device;
	inputInfo.sampler = m_textureSampler;
	inputInfo.shaderBits = VK_SHADER_STAGE_FRAGMENT_BIT;

	DescriptorInfo descInfo = {};
	descInfo.bindingSlot = 0u;
	descInfo.descriptorCount = static_cast<std::uint32_t>(m_textures.size());
	descInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	inputInfo.descriptorInfo = descInfo;

	for (auto& texture : m_textures) {
		inputInfo.imageView = texture->GetImageView();

		Terra::descriptorSet->AddSetLayout(inputInfo);
	}
}

void TextureStorage::RecordUploads(VkDevice device, VkCommandBuffer copyCmdBuffer) noexcept {
	m_uploadBuffers->FlushMemory(device);

	const auto& uploadBuffers = m_uploadBuffers->GetUploadBuffers();

	for (size_t index = 0u; index < m_textureData.size(); ++index)
		m_textures[index]->CopyToImage(
			copyCmdBuffer, uploadBuffers[index]->GetBuffer(),
			m_textureData[index].width, m_textureData[index].height
		);
}

void TextureStorage::TransitionImages(VkCommandBuffer graphicsBuffer) noexcept {
	for (auto& texture : m_textures)
		texture->TransitionImageLayout(graphicsBuffer, true);
}
