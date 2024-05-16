#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_
#include <VkResources.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkCommandQueue.hpp>
#include <deque>
#include <type_traits>
#include <optional>

// This class will store the texture. A texture added here will also be added to the manager at first.
// But can be removed and re-added.
class TextureStorage
{
public:
	TextureStorage(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_defaultSampler{ device }, m_textures{}, m_availableIndices{}, m_transitionQueue{},
		m_textureData{}, m_textureBindingIndices{}
	{
		m_defaultSampler.Create(VkSamplerCreateInfoBuilder{});
	}

	[[nodiscard]]
	size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height,
		StagingBufferManager& stagingBufferManager
	);

	void RemoveTexture(size_t index);

	void SetBindingIndex(size_t textureIndex, std::uint32_t bindingIndex) noexcept;

	[[nodiscard]]
	const VkTextureView& Get(size_t index) const noexcept
	{
		return m_textures.at(index);
	}
	[[nodiscard]]
	VkTextureView const* GetPtr(size_t index) const noexcept
	{
		return &m_textures.at(index);
	}

	[[nodiscard]]
	std::uint32_t GetBindingIndex(size_t textureIndex) const noexcept
	{
		// The plan is to not initialise the textureBindingIndices container, if we
		// aren't using the remove and re-adding binding feature. As a texture will be
		// added initially and if that feature isn't used, then storing the indices will
		// be just a waste of space. So, if the textureBindingIndices isn't populated, that
		// will mean the every single texture here is also bound. So, their indices should
		// be the same.
		if (std::size(m_textureBindingIndices) > textureIndex)
			return m_textureBindingIndices.at(textureIndex);
		else
			return static_cast<std::uint32_t>(textureIndex);
	}

	[[nodiscard]]
	const VKSampler& GetDefaultSampler() const noexcept
	{
		return m_defaultSampler;
	}

	[[nodiscard]]
	VKSampler const* GetDefaultSamplerPtr() const noexcept
	{
		return &m_defaultSampler;
	}

	void CleanupTempData() noexcept;
	void TransitionQueuedTextures(VKCommandBuffer& graphicsCmdBuffer);

private:
	VkDevice                                   m_device;
	MemoryManager*                             m_memoryManager;
	VKSampler                                  m_defaultSampler;
	// The TextureView objects need to have the same address until their data is copied.
	std::deque<VkTextureView>                  m_textures;
	std::vector<bool>                          m_availableIndices;
	std::queue<VkTextureView const*>           m_transitionQueue;
	std::vector<std::unique_ptr<std::uint8_t>> m_textureData;
	std::vector<std::uint32_t>                 m_textureBindingIndices;

	static constexpr VkFormat s_textureFormat = VK_FORMAT_R8G8B8A8_SRGB;

public:
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;

	TextureStorage(TextureStorage& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_defaultSampler{ std::move(other.m_defaultSampler) },
		m_textures{ std::move(other.m_textures) },
		m_availableIndices{ std::move(other.m_availableIndices) },
		m_transitionQueue{ std::move(other.m_transitionQueue) },
		m_textureData{ std::move(other.m_textureData) },
		m_textureBindingIndices{ std::move(other.m_textureBindingIndices) }
	{}
	TextureStorage& operator=(TextureStorage&& other) noexcept
	{
		m_device                = other.m_device;
		m_memoryManager         = other.m_memoryManager;
		m_defaultSampler        = std::move(other.m_defaultSampler);
		m_textures              = std::move(other.m_textures);
		m_availableIndices      = std::move(other.m_availableIndices);
		m_transitionQueue       = std::move(other.m_transitionQueue);
		m_textureData           = std::move(other.m_textureData);
		m_textureBindingIndices = std::move(other.m_textureBindingIndices);

		return *this;
	}
};

// This class will decide which of the textures will be bound to the pipeline. Every texture from
// the TextureStorage might not be bound at the same time.
class TextureManager
{
	static constexpr std::uint32_t s_textureDescriptorCount = std::numeric_limits<std::uint16_t>::max();

public:
	TextureManager() : m_availableIndices{} {}

	[[nodiscard]]
	std::optional<std::uint32_t> AddSampledTextureForBinding(
		VkDescriptorBuffer& descriptorBuffer, VkTextureView const* texture
	) noexcept;
	[[nodiscard]]
	std::optional<std::uint32_t> AddCombinedTextureForBinding(
		VkDescriptorBuffer& descriptorBuffer, VkTextureView const* texture, VKSampler const* sampler
	) noexcept;

	void RemoveTextureFromBinding(size_t index)
	{
		m_availableIndices.at(index) = true;
	}

	// Add s_textureDescriptorCount new entries to the available indices container. After calling
	// this, the descriptor buffer needs to be updated and recreated.
	void IncreaseAvailableIndices() noexcept;

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t texturesBindingSlot
	) const noexcept;

private:
	std::vector<bool> m_availableIndices;

public:
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	TextureManager(TextureManager&& other) noexcept
		: m_availableIndices{ std::move(other.m_availableIndices) }
	{}
	TextureManager& operator=(TextureManager&& other) noexcept
	{
		m_availableIndices = std::move(other.m_availableIndices);

		return *this;
	}
};
#endif
