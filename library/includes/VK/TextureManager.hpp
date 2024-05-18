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
		m_defaultSampler{ device }, m_textures{}, m_samplers{}, m_availableTextureIndices{},
		m_availableSamplerIndices{}, m_transitionQueue{}, m_textureData{}, m_textureBindingIndices{},
		m_samplerBindingIndices{}
	{
		m_defaultSampler.Create(VkSamplerCreateInfoBuilder{});
	}

	[[nodiscard]]
	size_t AddTexture(
		std::unique_ptr<std::uint8_t> textureData, size_t width, size_t height,
		StagingBufferManager& stagingBufferManager
	);
	[[nodiscard]]
	size_t AddSampler(const VkSamplerCreateInfoBuilder& builder);

	void RemoveTexture(size_t index);
	void RemoveSampler(size_t index);

	void SetTextureBindingIndex(size_t textureIndex, std::uint32_t bindingIndex) noexcept
	{
		SetBindingIndex(textureIndex, bindingIndex, m_textureBindingIndices);
	}
	void SetSamplerBindingIndex(size_t samplerIndex, std::uint32_t bindingIndex) noexcept
	{
		SetBindingIndex(samplerIndex, bindingIndex, m_samplerBindingIndices);
	}

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
	std::uint32_t GetTextureBindingIndex(size_t textureIndex) const noexcept
	{
		return GetBindingIndex(textureIndex, m_textureBindingIndices);
	}
	[[nodiscard]]
	std::uint32_t GetSamplerBindingIndex(size_t samplerIndex) const noexcept
	{
		return GetBindingIndex(samplerIndex, m_samplerBindingIndices);
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

	[[nodiscard]]
	const VKSampler& GetSampler(size_t index) const noexcept
	{
		return m_samplers.at(index);
	}

	[[nodiscard]]
	VKSampler const* GetSamplerPtr(size_t index) const noexcept
	{
		return &m_samplers.at(index);
	}

	void CleanupTempData() noexcept;
	void TransitionQueuedTextures(VKCommandBuffer& graphicsCmdBuffer);

private:
	[[nodiscard]]
	static std::uint32_t GetBindingIndex(
		size_t index, const std::vector<std::uint32_t>& bindingIndices
	) noexcept {
		// The plan is to not initialise the bindingIndices container, if we
		// aren't using the remove and re-adding binding feature. As an element will be
		// added initially and if that feature isn't used, then storing the indices will
		// be just a waste of space. So, if the bindingIndices isn't populated, that
		// will mean the every single element here is also bound. So, their indices should
		// be the same.
		if (std::size(bindingIndices) > index)
			return bindingIndices.at(index);
		else
			return static_cast<std::uint32_t>(index);
	}

	static void SetBindingIndex(
		size_t index, std::uint32_t bindingIndex, std::vector<std::uint32_t>& bindingIndices
	) noexcept;

private:
	VkDevice                                   m_device;
	MemoryManager*                             m_memoryManager;
	VKSampler                                  m_defaultSampler;
	// The TextureView objects need to have the same address until their data is copied.
	std::deque<VkTextureView>                  m_textures;
	std::deque<VKSampler>                      m_samplers;
	std::vector<bool>                          m_availableTextureIndices;
	std::vector<bool>                          m_availableSamplerIndices;
	std::queue<VkTextureView const*>           m_transitionQueue;
	std::vector<std::unique_ptr<std::uint8_t>> m_textureData;
	std::vector<std::uint32_t>                 m_textureBindingIndices;
	std::vector<std::uint32_t>                 m_samplerBindingIndices;

	static constexpr VkFormat s_textureFormat = VK_FORMAT_R8G8B8A8_SRGB;

public:
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;

	TextureStorage(TextureStorage& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_defaultSampler{ std::move(other.m_defaultSampler) },
		m_textures{ std::move(other.m_textures) },
		m_samplers{ std::move(other.m_samplers) },
		m_availableTextureIndices{ std::move(other.m_availableTextureIndices) },
		m_availableSamplerIndices{ std::move(other.m_availableSamplerIndices) },
		m_transitionQueue{ std::move(other.m_transitionQueue) },
		m_textureData{ std::move(other.m_textureData) },
		m_textureBindingIndices{ std::move(other.m_textureBindingIndices) },
		m_samplerBindingIndices{ std::move(other.m_samplerBindingIndices) }
	{}
	TextureStorage& operator=(TextureStorage&& other) noexcept
	{
		m_device                  = other.m_device;
		m_memoryManager           = other.m_memoryManager;
		m_defaultSampler          = std::move(other.m_defaultSampler);
		m_textures                = std::move(other.m_textures);
		m_samplers                = std::move(other.m_samplers);
		m_availableTextureIndices = std::move(other.m_availableTextureIndices);
		m_availableSamplerIndices = std::move(other.m_availableSamplerIndices);
		m_transitionQueue         = std::move(other.m_transitionQueue);
		m_textureData             = std::move(other.m_textureData);
		m_textureBindingIndices   = std::move(other.m_textureBindingIndices);
		m_samplerBindingIndices   = std::move(other.m_samplerBindingIndices);

		return *this;
	}
};

enum class TextureDescType
{
	CombinedTexture,
	SampledTexture,
	Sampler
};

// This class will decide which of the textures will be bound to the pipeline. Every texture from
// the TextureStorage might not be bound at the same time.
class TextureManager
{
	static constexpr std::uint32_t s_combinedTextureDescriptorCount
		= std::numeric_limits<std::uint16_t>::max();
	static constexpr std::uint32_t s_sampledTextureDescriptorCount
		= std::numeric_limits<std::uint8_t>::max();
	static constexpr std::uint32_t s_samplerDescriptorCount
		= std::numeric_limits<std::uint8_t>::max();

public:
	TextureManager()
		: m_availableIndicesCombinedTextures( s_combinedTextureDescriptorCount, true ),
		m_availableIndicesSampledTextures{}, m_availableIndicesSamplers{}
	{}

	[[nodiscard]]
	std::optional<std::uint32_t> AddSampledTextureForBinding(
		VkDescriptorBuffer& descriptorBuffer, VkTextureView const* texture,
		std::uint32_t sampledTexturesBindingSlot
	) noexcept;
	[[nodiscard]]
	std::optional<std::uint32_t> AddSamplerForBinding(
		VkDescriptorBuffer& descriptorBuffer, VKSampler const* sampler,
		std::uint32_t samplersBindingSlot
	) noexcept;
	[[nodiscard]]
	std::optional<std::uint32_t> AddCombinedTextureForBinding(
		VkDescriptorBuffer& descriptorBuffer, VkTextureView const* texture, VKSampler const* sampler,
		std::uint32_t combinedTexturesBindingSlot
	) noexcept;

	void RemoveFromBinding(size_t index, TextureDescType descType)
	{
		if (descType == TextureDescType::CombinedTexture)
			m_availableIndicesCombinedTextures.at(index) = true;
		else if (descType == TextureDescType::SampledTexture)
			m_availableIndicesSampledTextures.at(index) = true;
		else if (descType == TextureDescType::Sampler)
			m_availableIndicesSamplers.at(index) = true;
	}

	// Add new entries to the available indices container. After calling
	// this, the descriptor buffer needs to be updated and recreated.
	void IncreaseAvailableIndices(TextureDescType descType) noexcept;

	void SetDescriptorBuffer(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t combinedTexturesBindingSlot,
		std::uint32_t sampledTexturesBindingSlot, std::uint32_t samplersBindingSlot
	) const noexcept;

private:
	[[nodiscard]]
	static std::optional<std::uint32_t> FindFreeIndex(
		const std::vector<bool>& availableIndices
	) noexcept;

private:
	std::vector<bool> m_availableIndicesCombinedTextures;
	std::vector<bool> m_availableIndicesSampledTextures;
	std::vector<bool> m_availableIndicesSamplers;

public:
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	TextureManager(TextureManager&& other) noexcept
		: m_availableIndicesCombinedTextures{ std::move(other.m_availableIndicesCombinedTextures) },
		m_availableIndicesSampledTextures{ std::move(other.m_availableIndicesSampledTextures) },
		m_availableIndicesSamplers{ std::move(other.m_availableIndicesSamplers) }
	{}
	TextureManager& operator=(TextureManager&& other) noexcept
	{
		m_availableIndicesCombinedTextures = std::move(other.m_availableIndicesCombinedTextures);
		m_availableIndicesSampledTextures  = std::move(other.m_availableIndicesSampledTextures);
		m_availableIndicesSamplers         = std::move(other.m_availableIndicesSamplers);

		return *this;
	}
};
#endif
