#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_
#include <VkResources.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>
#include <VkCommandQueue.hpp>
#include <TemporaryDataBuffer.hpp>
#include <ranges>
#include <algorithm>
#include <deque>
#include <type_traits>
#include <optional>
#include <Texture.hpp>

// This class will store the texture. A texture added here will also be added to the manager at first.
// But can be removed and re-added.
class TextureStorage
{
	static constexpr size_t s_defaultSamplerIndex = 0u;
public:
	TextureStorage(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_memoryManager{ memoryManager },
		m_textures{}, m_samplers{}, m_availableTextureIndices{},
		m_availableSamplerIndices{}, m_transitionQueue{}, m_textureBindingIndices{},
		m_samplerBindingIndices{}
	{
		VKSampler& defaultSampler = m_samplers.emplace_back(device);
		defaultSampler.Create(VkSamplerCreateInfoBuilder{});
	}

	[[nodiscard]]
	size_t AddTexture(
		STexture&& texture, StagingBufferManager& stagingBufferManager, TemporaryDataBufferGPU& tempBuffer
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
	static constexpr std::uint32_t GetDefaultSamplerIndex() noexcept
	{
		return static_cast<std::uint32_t>(s_defaultSamplerIndex);
	}

	[[nodiscard]]
	const VKSampler& GetDefaultSampler() const noexcept
	{
		return m_samplers.at(s_defaultSamplerIndex);
	}

	[[nodiscard]]
	VKSampler const* GetDefaultSamplerPtr() const noexcept
	{
		return &m_samplers.at(s_defaultSamplerIndex);
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

	// I could have used the AcquireOwnership function to do the layout transition. But there are
	// two reasons, well one in this case to make this extra transition. If the resource has shared
	// ownership, the AcquireOwnership function wouldn't be called. In this class all of the textures
	// have exclusive ownership. But if the transfer and graphics queues have the same family, then
	// the ownership transfer wouldn't be necessary.
	void TransitionQueuedTextures(const VKCommandBuffer& graphicsCmdBuffer);

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
	VkDevice                         m_device;
	MemoryManager*                   m_memoryManager;
	// The TextureView objects need to have the same address until their data is copied.
	std::deque<VkTextureView>        m_textures;
	std::deque<VKSampler>            m_samplers;
	std::vector<bool>                m_availableTextureIndices;
	std::vector<bool>                m_availableSamplerIndices;
	std::queue<VkTextureView const*> m_transitionQueue;
	std::vector<std::uint32_t>       m_textureBindingIndices;
	std::vector<std::uint32_t>       m_samplerBindingIndices;

	static constexpr VkFormat s_textureFormat = VK_FORMAT_R8G8B8A8_SRGB;

public:
	TextureStorage(const TextureStorage&) = delete;
	TextureStorage& operator=(const TextureStorage&) = delete;

	TextureStorage(TextureStorage&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_textures{ std::move(other.m_textures) },
		m_samplers{ std::move(other.m_samplers) },
		m_availableTextureIndices{ std::move(other.m_availableTextureIndices) },
		m_availableSamplerIndices{ std::move(other.m_availableSamplerIndices) },
		m_transitionQueue{ std::move(other.m_transitionQueue) },
		m_textureBindingIndices{ std::move(other.m_textureBindingIndices) },
		m_samplerBindingIndices{ std::move(other.m_samplerBindingIndices) }
	{}
	TextureStorage& operator=(TextureStorage&& other) noexcept
	{
		m_device                  = other.m_device;
		m_memoryManager           = other.m_memoryManager;
		m_textures                = std::move(other.m_textures);
		m_samplers                = std::move(other.m_samplers);
		m_availableTextureIndices = std::move(other.m_availableTextureIndices);
		m_availableSamplerIndices = std::move(other.m_availableSamplerIndices);
		m_transitionQueue         = std::move(other.m_transitionQueue);
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
	static constexpr std::uint32_t s_localDescriptorCount
		= std::numeric_limits<std::uint8_t>::max();

	// I will keep all the Fragment shader resources in a different layout.
	// So, the local count being 1 is fine.
	static constexpr std::uint32_t localSetLayoutCount = 1u;

public:
	TextureManager(VkDevice device, MemoryManager* memoryManager)
		: m_localDescBuffer{ device, memoryManager, localSetLayoutCount }, m_inactiveCombinedDescDetails{},
		m_inactiveSampledDescDetails{}, m_inactiveSamplerDescDetails{},
		m_availableIndicesCombinedTextures(s_combinedTextureDescriptorCount, true),
		m_availableIndicesSampledTextures{},
		m_availableIndicesSamplers{}, m_localCombinedDescCount{ 0u }, m_localSampledDescCount{ 0u },
		m_localSamplerDescCount{ 0u }
	{}

	// Add new entries to the available indices container. After calling
	// this, the descriptor buffer needs to be updated and recreated.
	void IncreaseAvailableIndices(TextureDescType descType) noexcept;

	void SetDescriptorBufferLayout(
		VkDescriptorBuffer& descriptorBuffer, std::uint32_t combinedTexturesBindingSlot,
		std::uint32_t sampledTexturesBindingSlot, std::uint32_t samplersBindingSlot,
		size_t setLayoutIndex
	) const noexcept;

public:
	struct DescDetailsCombined
	{
		std::uint32_t textureIndex;
		std::uint32_t samplerIndex;

		constexpr bool operator==(const DescDetailsCombined& other) const noexcept
		{
			return textureIndex == other.textureIndex && samplerIndex == other.samplerIndex;
		}
	};

private:
	[[nodiscard]]
	static std::optional<std::uint32_t> FindFreeIndex(
		const std::vector<bool>& availableIndices
	) noexcept;

private:
	// It will be used for caching descriptors.
	VkDescriptorBuffer               m_localDescBuffer;
	std::vector<DescDetailsCombined> m_inactiveCombinedDescDetails;
	std::vector<std::uint32_t>       m_inactiveSampledDescDetails;
	std::vector<std::uint32_t>       m_inactiveSamplerDescDetails;
	std::vector<bool>                m_availableIndicesCombinedTextures;
	std::vector<bool>                m_availableIndicesSampledTextures;
	std::vector<bool>                m_availableIndicesSamplers;
	std::uint32_t                    m_localCombinedDescCount;
	std::uint32_t                    m_localSampledDescCount;
	std::uint32_t                    m_localSamplerDescCount;

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
	const std::vector<bool>& GetAvailableIndices() const noexcept
	{
		if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			return m_availableIndicesSampledTextures;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			return m_availableIndicesSamplers;
		else
			return m_availableIndicesCombinedTextures;
	}

	template<VkDescriptorType type>
	std::vector<bool>& GetAvailableIndices() noexcept
	{
		if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			return m_availableIndicesSampledTextures;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			return m_availableIndicesSamplers;
		else
			return m_availableIndicesCombinedTextures;
	}

	template<VkDescriptorType type>
	auto& GetInactiveDetails() noexcept
	{
		if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			return m_inactiveSampledDescDetails;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			return m_inactiveSamplerDescDetails;
		else
			return m_inactiveCombinedDescDetails;
	}

	template<VkDescriptorType type>
	std::uint32_t& GetLocalDescCount() noexcept
	{
		if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			return m_localSampledDescCount;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			return m_localSamplerDescCount;
		else
			return m_localCombinedDescCount;
	}

public:
	template<VkDescriptorType type>
	void RemoveLocalDescriptor(std::uint32_t resourceIndex) noexcept
	{
		static_assert(
			type != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			"Use the specilised function for combined textures."
		);

		std::vector<std::uint32_t>& inactiveDescDetails = GetInactiveDetails<type>();

		std::erase(inactiveDescDetails, resourceIndex);
	}

	void RemoveCombinedLocalDescriptorTexture(std::uint32_t index) noexcept
	{
		auto result = std::ranges::remove(
			m_inactiveCombinedDescDetails, index,
			[](const DescDetailsCombined& details) { return details.textureIndex; }
		);
		m_inactiveCombinedDescDetails.erase(std::begin(result), std::end(result));
	}

	void RemoveCombinedLocalDescriptorSampler(std::uint32_t index) noexcept
	{
		auto result = std::ranges::remove(
			m_inactiveCombinedDescDetails, index,
			[](const DescDetailsCombined& details) { return details.samplerIndex; }
		);
		m_inactiveCombinedDescDetails.erase(std::begin(result), std::end(result));
	}

	template<VkDescriptorType type, typename T>
	[[nodiscard]]
	std::optional<void const*> GetLocalDescriptor(const T& descDetails) noexcept
	{
		constexpr TextureDescType descType  = GetTextureDescType<type>();
		const auto localBindingSlot         = static_cast<std::uint32_t>(descType);
		std::vector<T>& inactiveDescDetails = GetInactiveDetails<type>();

		auto result = std::ranges::find(inactiveDescDetails, descDetails);

		if (result != std::end(inactiveDescDetails))
		{
			const auto localDescIndex = static_cast<std::uint32_t>(
				std::distance(std::begin(inactiveDescDetails), result)
			);

			inactiveDescDetails.erase(result);

			return m_localDescBuffer.GetDescriptor<type>(localBindingSlot, 0u, localDescIndex);
		}

		return {};
	}

	template<VkDescriptorType type, typename T>
	void SetLocalDescriptor(void const* descriptor, const T& descDetails)
	{
		constexpr TextureDescType descType  = GetTextureDescType<type>();
		// The local buffer will have a binding for each descriptor type.
		const auto localBindingSlot         = static_cast<std::uint32_t>(descType);
		std::vector<T>& inactiveDescDetails = GetInactiveDetails<type>();
		std::uint32_t& localDescCount       = GetLocalDescCount<type>();

		// We add any new items to the end of the inactiveDetails. The actual buffer might
		// be able to house more items, as we will erase the inactive detail when it is active
		// so let's check if the buffer can house the new number of inactive details. If not
		// recreate the buffer to be bigger.
		auto localDescIndex = static_cast<std::uint32_t>(std::size(inactiveDescDetails));

		const std::uint32_t requiredCount = localDescIndex + 1u;

		if (localDescCount < requiredCount)
		{
			localDescCount += s_localDescriptorCount;

			m_localDescBuffer.AddBinding(
				localBindingSlot, 0u, type, localDescCount, VK_SHADER_STAGE_FRAGMENT_BIT
			);

			// Add binding will add a new binding to the Layout and we use the layout size to
			// create the buffer. So, we don't need to pass a size.
			m_localDescBuffer.RecreateBuffer();
		}

		m_localDescBuffer.SetDescriptor<type>(descriptor, localBindingSlot, 0u, localDescIndex);

		inactiveDescDetails.emplace_back(descDetails);
	}

	template<VkDescriptorType type>
	[[nodiscard]]
	// Returns the descriptor index if there is an available index. Otherwise, returns empty. In such
	// case, the available indices should be increased and the descriptor buffer should be recreated.
	// This will only work if this object is the only object which is managing all the texture
	// descriptors across all the descriptor buffers which are passed to it.
	std::optional<std::uint32_t> GetFreeGlobalDescriptorIndex() const noexcept
	{
		const std::vector<bool>& availableIndices = GetAvailableIndices<type>();

		return FindFreeIndex(availableIndices);
	}

	template<VkDescriptorType type>
	void SetAvailableIndex(std::uint32_t descriptorIndex, bool availablity) noexcept
	{
		std::vector<bool>& availableIndices = GetAvailableIndices<type>();

		if (std::size(availableIndices) > descriptorIndex)
			availableIndices.at(static_cast<size_t>(descriptorIndex)) = availablity;
	}

	// Use the global index to set the descriptor in the desired global descriptor buffer. If
	// there was a local descriptor, copy it, otherwise create a new descriptor.

public:
	TextureManager(const TextureManager&) = delete;
	TextureManager& operator=(const TextureManager&) = delete;

	TextureManager(TextureManager&& other) noexcept
		: m_localDescBuffer{ std::move(other.m_localDescBuffer) },
		m_inactiveCombinedDescDetails{ std::move(other.m_inactiveCombinedDescDetails) },
		m_inactiveSampledDescDetails{ std::move(other.m_inactiveSampledDescDetails) },
		m_inactiveSamplerDescDetails{ std::move(other.m_inactiveSamplerDescDetails) },
		m_availableIndicesCombinedTextures{ std::move(other.m_availableIndicesCombinedTextures) },
		m_availableIndicesSampledTextures{ std::move(other.m_availableIndicesSampledTextures) },
		m_availableIndicesSamplers{ std::move(other.m_availableIndicesSamplers) },
		m_localCombinedDescCount{ other.m_localCombinedDescCount },
		m_localSampledDescCount{ other.m_localSampledDescCount },
		m_localSamplerDescCount{ other.m_localSamplerDescCount }
	{}
	TextureManager& operator=(TextureManager&& other) noexcept
	{
		m_localDescBuffer                  = std::move(other.m_localDescBuffer);
		m_inactiveCombinedDescDetails      = std::move(other.m_inactiveCombinedDescDetails);
		m_inactiveSampledDescDetails       = std::move(other.m_inactiveSampledDescDetails);
		m_inactiveSamplerDescDetails       = std::move(other.m_inactiveSamplerDescDetails);
		m_availableIndicesCombinedTextures = std::move(other.m_availableIndicesCombinedTextures);
		m_availableIndicesSampledTextures  = std::move(other.m_availableIndicesSampledTextures);
		m_availableIndicesSamplers         = std::move(other.m_availableIndicesSamplers);
		m_localCombinedDescCount           = other.m_localCombinedDescCount;
		m_localSampledDescCount            = other.m_localSampledDescCount;
		m_localSamplerDescCount            = other.m_localSamplerDescCount;

		return *this;
	}
};
#endif
