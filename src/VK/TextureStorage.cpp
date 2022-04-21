#include <TextureStorage.hpp>
#include <VKThrowMacros.hpp>
#include <CRSMath.hpp>
#include <Terra.hpp>

TextureStorage::TextureStorage(
	VkDevice logicalDevice, VkPhysicalDevice physicalDevice,
	std::vector<std::uint32_t> queueFamilyIndices
) : m_currentOffset(0u), m_queueFamilyIndices(std::move(queueFamilyIndices)) {

	m_uploadBuffers = std::make_unique<UploadBuffers>(
		logicalDevice, physicalDevice
		);
	m_textureMemory = std::make_unique<DeviceMemory>(
		logicalDevice, physicalDevice, m_queueFamilyIndices, false, BufferType::Image
		);
}

size_t TextureStorage::AddTexture(
		VkDevice device,
		const void* data,
		size_t width, size_t height, size_t pixelSizeInBytes
) noexcept {
	m_textureData.emplace_back(
		static_cast<std::uint32_t>(width),
		static_cast<std::uint32_t>(height),
		m_currentOffset
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
		static_cast<std::uint32_t>(pixelSizeInBytes),
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

	VkResult result;
	for (size_t index = 0u; index < m_textures.size(); ++index)
		VK_THROW_FAILED(result,
			vkBindImageMemory(
				device, m_textures[index]->GetImage(),
				textureMemory, m_textureData[index].offset
			)
		);
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
