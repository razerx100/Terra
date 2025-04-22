#ifndef VK_TEXTURE_MANAGER_HPP_
#define VK_TEXTURE_MANAGER_HPP_
#include <VkResources.hpp>
#include <VkStagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkCommandQueue.hpp>
#include <TemporaryDataBuffer.hpp>
#include <ReusableVector.hpp>
#include <ranges>
#include <algorithm>
#include <deque>
#include <type_traits>
#include <optional>
#include <Texture.hpp>

namespace Terra
{
// This class will store the texture. A texture added here will also be added to the manager at first.
// But can be removed and re-added.
class TextureStorage
{
	inline static size_t s_defaultSamplerIndex = 0u;
public:
	TextureStorage(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_textures{}, m_samplers{}, m_transitionQueue{}, m_combinedCacheDetails{}
	{
		VKSampler defaultSampler{ device };
		defaultSampler.Create(VkSamplerCreateInfoBuilder{});

		// This should always be 0 but still doing this to please the compiler.
		s_defaultSamplerIndex = m_samplers.Add(std::move(defaultSampler));
	}

	[[nodiscard]]
	size_t AddTexture(
		STexture&& texture, StagingBufferManager& stagingBufferManager,
		Callisto::TemporaryDataBufferGPU& tempBuffer
	);
	[[nodiscard]]
	size_t AddSampler(const VkSamplerCreateInfoBuilder& builder);

	void RemoveTexture(size_t index);
	void RemoveSampler(size_t index);

	void SetCombinedCacheDetails(
		std::uint32_t textureIndex, std::uint32_t samplerIndex, std::uint32_t localDescIndex
	) noexcept;

	[[nodiscard]]
	std::optional<std::uint32_t> GetAndRemoveCombinedLocalDescIndex(
		std::uint32_t textureIndex, std::uint32_t samplerIndex
	) noexcept;

	[[nodiscard]]
	std::vector<std::uint32_t> GetAndRemoveCombinedCacheDetailsTexture(
		std::uint32_t textureIndex
	) noexcept;
	[[nodiscard]]
	std::vector<std::uint32_t> GetAndRemoveCombinedCacheDetailsSampler(
		std::uint32_t samplerIndex
	) noexcept;

	[[nodiscard]]
	const VkTextureView& Get(size_t index) const noexcept
	{
		return m_textures[index];
	}
	[[nodiscard]]
	VkTextureView const* GetPtr(size_t index) const noexcept
	{
		return &m_textures[index];
	}

	[[nodiscard]]
	static constexpr std::uint32_t GetDefaultSamplerIndex() noexcept
	{
		return static_cast<std::uint32_t>(s_defaultSamplerIndex);
	}

	[[nodiscard]]
	const VKSampler& GetDefaultSampler() const noexcept
	{
		return m_samplers[s_defaultSamplerIndex];
	}

	[[nodiscard]]
	VKSampler const* GetDefaultSamplerPtr() const noexcept
	{
		return &m_samplers[s_defaultSamplerIndex];
	}

	[[nodiscard]]
	const VKSampler& GetSampler(size_t index) const noexcept
	{
		return m_samplers[index];
	}

	[[nodiscard]]
	VKSampler const* GetSamplerPtr(size_t index) const noexcept
	{
		return &m_samplers[index];
	}

	// I could have used the AcquireOwnership function to do the layout transition. But there are
	// two reasons, well one in this case to make this extra transition. If the resource has shared
	// ownership, the AcquireOwnership function wouldn't be called. In this class all of the textures
	// have exclusive ownership. But if the transfer and graphics queues have the same family, then
	// the ownership transfer wouldn't be necessary.
	void TransitionQueuedTextures(const VKCommandBuffer& graphicsCmdBuffer);

private:
	struct CombinedCacheDetails
	{
		std::uint32_t textureIndex;
		std::uint32_t samplerIndex;
		std::uint32_t localDescIndex;

		constexpr bool operator==(const CombinedCacheDetails& other) const noexcept
		{
			return textureIndex   == other.textureIndex
				&& samplerIndex   == other.samplerIndex
				&& localDescIndex == other.localDescIndex;
		}
	};

private:
	VkDevice                               m_device;
	MemoryManager*                         m_memoryManager;
	// The TextureView objects need to have the same address until their data is copied.
	// For the transitionQueue member and also the StagingBufferManager.
	Callisto::ReusableDeque<VkTextureView> m_textures;
	Callisto::ReusableDeque<VKSampler>     m_samplers;
	std::queue<VkTextureView const*>       m_transitionQueue;

	std::vector<CombinedCacheDetails>      m_combinedCacheDetails;

	static constexpr VkFormat s_textureFormat = VK_FORMAT_R8G8B8A8_SRGB;

public:
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;

	TextureStorage(TextureStorage&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_textures{ std::move(other.m_textures) },
		m_samplers{ std::move(other.m_samplers) },
		m_transitionQueue{ std::move(other.m_transitionQueue) },
		m_combinedCacheDetails{ std::move(other.m_combinedCacheDetails) }
	{}
	TextureStorage& operator=(TextureStorage&& other) noexcept
	{
		m_device               = other.m_device;
		m_memoryManager        = other.m_memoryManager;
		m_textures             = std::move(other.m_textures);
		m_samplers             = std::move(other.m_samplers);
		m_transitionQueue      = std::move(other.m_transitionQueue);
		m_combinedCacheDetails = std::move(other.m_combinedCacheDetails);

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
	static constexpr std::uint32_t s_localDescriptorCount
		= std::numeric_limits<std::uint8_t>::max();

	// I will keep all the Fragment shader resources in a different layout.
	// So, the local count being 1 is fine.
	static constexpr std::uint32_t localSetLayoutCount = 1u;

public:
	TextureManager(VkDevice device, MemoryManager* memoryManager)
		: m_availableIndicesCombinedTextures(s_combinedTextureDescriptorCount),
		m_availableIndicesSampledTextures{}, m_availableIndicesSamplers{},
		m_localDescBuffer{ device, memoryManager, localSetLayoutCount },
		m_combinedTextureCaches{}, m_sampledTextureCaches{}, m_samplerCaches{}
	{}

	void SetDescriptorBufferLayout(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t combinedTexturesBindingSlot,
		std::uint32_t sampledTexturesBindingSlot, std::uint32_t samplersBindingSlot,
		size_t setLayoutIndex
	) const noexcept;

private:
	Callisto::IndicesManager m_availableIndicesCombinedTextures;
	Callisto::IndicesManager m_availableIndicesSampledTextures;
	Callisto::IndicesManager m_availableIndicesSamplers;

	// It will be used for caching descriptors.
	VkDescriptorBuffer       m_localDescBuffer;
	Callisto::IndicesManager m_combinedTextureCaches;
	Callisto::IndicesManager m_sampledTextureCaches;
	Callisto::IndicesManager m_samplerCaches;

private:
	template<VkDescriptorType type>
	[[nodiscard]]
	static constexpr TextureDescType GetTextureDescType() noexcept
	{
		if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			return TextureDescType::SampledTexture;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			return TextureDescType::Sampler;
		else
			return TextureDescType::CombinedTexture;
	}

	template<VkDescriptorType type>
	[[nodiscard]]
	auto&& GetAvailableBindings(this auto&& self) noexcept
	{
		if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			return std::forward_like<decltype(self)>(self.m_availableIndicesSampledTextures);
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			return std::forward_like<decltype(self)>(self.m_availableIndicesSamplers);
		else
			return std::forward_like<decltype(self)>(self.m_availableIndicesCombinedTextures);
	}

	template<VkDescriptorType type>
	[[nodiscard]]
	auto&& GetCaches(this auto&& self) noexcept
	{
		if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			return std::forward_like<decltype(self)>(self.m_sampledTextureCaches);
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			return std::forward_like<decltype(self)>(self.m_samplerCaches);
		else
			return std::forward_like<decltype(self)>(self.m_combinedTextureCaches);
	}

public:
	// Add new entries to the available indices container. After calling
	// this, the descriptor buffer needs to be updated and recreated.
	template<TextureDescType DescType>
	void IncreaseMaximumBindingCount() noexcept
	{
		if constexpr (DescType == TextureDescType::CombinedTexture)
		{
			const size_t newSize =
				std::size(m_availableIndicesCombinedTextures) + s_combinedTextureDescriptorCount;
			m_availableIndicesCombinedTextures.Resize(newSize);
		}
		else if constexpr (DescType == TextureDescType::SampledTexture)
		{
			const size_t newSize =
				std::size(m_availableIndicesSampledTextures) + s_sampledTextureDescriptorCount;
			m_availableIndicesSampledTextures.Resize(newSize);
		}
		else if constexpr (DescType == TextureDescType::Sampler)
		{
			const size_t newSize = std::size(m_availableIndicesSamplers) + s_samplerDescriptorCount;
			m_availableIndicesSamplers.Resize(newSize);
		}
	}

	template<VkDescriptorType type>
	void SetLocalDescriptorAvailability(std::uint32_t localDescIndex, bool availability) noexcept
	{
		Callisto::IndicesManager& localCaches = GetCaches<type>();

		localCaches.ToggleAvailability(localDescIndex, availability);
	}

	template<VkDescriptorType type>
	std::uint32_t GetFirstFreeLocalDescriptor()
	{
		constexpr TextureDescType descType = GetTextureDescType<type>();
		const auto localBindingSlot        = static_cast<std::uint32_t>(descType);

		Callisto::IndicesManager& localCaches = GetCaches<type>();

		std::optional<size_t> oFreeIndex = localCaches.GetFirstAvailableIndex();

		if (!oFreeIndex)
		{
			const auto newLocalDescCount = static_cast<std::uint32_t>(
				std::size(localCaches) + s_localDescriptorCount
			);

			localCaches.Resize(newLocalDescCount);

			oFreeIndex = localCaches.GetFirstAvailableIndex();

			// Extended the desc buffer.
			constexpr size_t setLayoutIndex = 0u;

			const std::vector<VkDescriptorSetLayoutBinding> oldSetLayoutBindings
				= m_localDescBuffer.GetLayout(setLayoutIndex).GetBindings();

			m_localDescBuffer.AddBinding(
				localBindingSlot, setLayoutIndex, type, newLocalDescCount, VK_SHADER_STAGE_FRAGMENT_BIT
			);

			// Add binding will add a new binding to the Layout and we use the layout size to
			// create the buffer. So, we don't need to pass a size.
			if (m_localDescBuffer.IsCreated())
				m_localDescBuffer.RecreateSetLayout(setLayoutIndex, oldSetLayoutBindings);
			else
				m_localDescBuffer.CreateBuffer();
		}

		return static_cast<std::uint32_t>(oFreeIndex.value());
	}

	template<VkDescriptorType type>
	[[nodiscard]]
	void const* GetLocalDescriptor(std::uint32_t localDescIndex) noexcept
	{
		constexpr TextureDescType descType = GetTextureDescType<type>();
		const auto localBindingSlot        = static_cast<std::uint32_t>(descType);

		return m_localDescBuffer.GetDescriptor<type>(localBindingSlot, 0u, localDescIndex);
	}

	template<VkDescriptorType type>
	void SetLocalDescriptor(void const* descriptor, std::uint32_t localDescIndex)
	{
		constexpr TextureDescType descType = GetTextureDescType<type>();
		// The local buffer will have a binding for each descriptor type.
		const auto localBindingSlot        = static_cast<std::uint32_t>(descType);

		m_localDescBuffer.SetDescriptor<type>(descriptor, localBindingSlot, 0u, localDescIndex);
	}

	// Returns the descriptor index if there is an available index. Otherwise, returns empty. In such
	// case, the available indices should be increased and the descriptor buffer should be recreated.
	// This will only work if this object is the only object which is managing all the texture
	// descriptors across all the descriptor buffers which are passed to it.
	template<VkDescriptorType type>
	[[nodiscard]]
	std::optional<size_t> GetFreeGlobalDescriptorIndex() const noexcept
	{
		const Callisto::IndicesManager& availableBindings = GetAvailableBindings<type>();

		return availableBindings.GetFirstAvailableIndex();
	}

	template<VkDescriptorType type>
	void SetBindingAvailability(std::uint32_t descriptorIndex, bool availability) noexcept
	{
		Callisto::IndicesManager& availableBindings = GetAvailableBindings<type>();

		if (std::size(availableBindings) > descriptorIndex)
			availableBindings.ToggleAvailability(static_cast<size_t>(descriptorIndex), availability);
	}

	// Use the global index to set the descriptor in the desired global descriptor buffer. If
	// there was a local descriptor, copy it, otherwise create a new descriptor.

public:
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	TextureManager(TextureManager&& other) noexcept
		: m_availableIndicesCombinedTextures{ std::move(other.m_availableIndicesCombinedTextures) },
		m_availableIndicesSampledTextures{ std::move(other.m_availableIndicesSampledTextures) },
		m_availableIndicesSamplers{ std::move(other.m_availableIndicesSamplers) },
		m_localDescBuffer{ std::move(other.m_localDescBuffer) },
		m_combinedTextureCaches{ std::move(other.m_combinedTextureCaches) },
		m_sampledTextureCaches{ std::move(other.m_sampledTextureCaches) },
		m_samplerCaches{ std::move(other.m_samplerCaches) }
	{}
	TextureManager& operator=(TextureManager&& other) noexcept
	{
		m_availableIndicesCombinedTextures = std::move(other.m_availableIndicesCombinedTextures);
		m_availableIndicesSampledTextures  = std::move(other.m_availableIndicesSampledTextures);
		m_availableIndicesSamplers         = std::move(other.m_availableIndicesSamplers);
		m_localDescBuffer                  = std::move(other.m_localDescBuffer);
		m_combinedTextureCaches            = std::move(other.m_combinedTextureCaches);
		m_sampledTextureCaches             = std::move(other.m_sampledTextureCaches);
		m_samplerCaches                    = std::move(other.m_samplerCaches);

		return *this;
	}
};
}
#endif
