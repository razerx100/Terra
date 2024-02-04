#ifndef VK_TEXTURE_VIEW_HPP_
#define VK_TEXTURE_VIEW_HPP_
#include <VkResources.hpp>

class VkTextureView
{
public:
	VkTextureView(VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType)
		: m_device{ device }, m_imageView{ VK_NULL_HANDLE },
		m_texture{ device, memoryManager, memoryType }
	{}
	~VkTextureView() noexcept;

	void CreateView(
		std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		VkImageUsageFlags textureUsageFlags, VkImageAspectFlags aspectFlags,
		VkImageViewType imageType, std::uint32_t mipLevel, std::uint32_t mipLevelCount,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void CreateView(
		std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		VkImageUsageFlags textureUsageFlags, VkImageAspectFlags aspectFlags,
		VkImageViewType imageType, const std::vector<std::uint32_t>& queueFamilyIndices
	);

	[[nodiscard]]
	const Texture& GetTexture() const noexcept { return m_texture; }
	[[nodiscard]]
	VkImageView GetView() const noexcept { return m_imageView; }

private:
	void CreateImageView(
		VkImageAspectFlags aspectFlags, VkImageViewType imageType,
		std::uint32_t mipLevel, std::uint32_t mipLevelCount
	);

private:
	VkDevice    m_device;
	VkImageView m_imageView;
	Texture     m_texture;

public:
	VkTextureView(const VkTextureView&) = delete;
	VkTextureView& operator=(const VkTextureView&) = delete;

	VkTextureView(VkTextureView&& other) noexcept
		: m_device{ other.m_device }, m_imageView{ other.m_imageView },
		m_texture{ std::move(other.m_texture) }
	{
		other.m_imageView = VK_NULL_HANDLE;
	}
	VkTextureView& operator=(VkTextureView&& other) noexcept
	{
		m_device          = other.m_device;
		m_imageView       = other.m_imageView;
		m_texture         = std::move(other.m_texture);
		other.m_imageView = VK_NULL_HANDLE;

		return *this;
	}
};

class VkSamplerCreateInfoBuilder
{
public:
	VkSamplerCreateInfoBuilder();

	static void SetMaxAnisotropy(VkPhysicalDevice physicalDevice) noexcept;

	VkSamplerCreateInfoBuilder& Anisotropy(float anisotropy) noexcept;
	VkSamplerCreateInfoBuilder& MinFilter(VkFilter minFilter) noexcept
	{
		m_createInfo.minFilter = minFilter;

		return *this;
	}
	VkSamplerCreateInfoBuilder& MagFilter(VkFilter magFilter) noexcept
	{
		m_createInfo.magFilter = magFilter;

		return *this;
	}
	VkSamplerCreateInfoBuilder& AddressU(VkSamplerAddressMode mode) noexcept
	{
		m_createInfo.addressModeU = mode;

		return *this;
	}
	VkSamplerCreateInfoBuilder& AddressV(VkSamplerAddressMode mode) noexcept
	{
		m_createInfo.addressModeV = mode;

		return *this;
	}
	VkSamplerCreateInfoBuilder& AddressW(VkSamplerAddressMode mode) noexcept
	{
		m_createInfo.addressModeW = mode;

		return *this;
	}
	VkSamplerCreateInfoBuilder& CompareOp(VkCompareOp compare) noexcept
	{
		m_createInfo.compareEnable = VK_TRUE;
		m_createInfo.compareOp     = compare;

		return *this;
	}
	VkSamplerCreateInfoBuilder& MipLOD(float value) noexcept
	{
		m_createInfo.mipLodBias = value;

		return *this;
	}
	VkSamplerCreateInfoBuilder& MinLOD(float value) noexcept
	{
		m_createInfo.minLod = value;

		return *this;
	}
	VkSamplerCreateInfoBuilder& MaxLOD(float value) noexcept
	{
		m_createInfo.maxLod = value;

		return *this;
	}
	VkSamplerCreateInfoBuilder& BorderColour(VkBorderColor colour) noexcept
	{
		m_createInfo.borderColor = colour;

		return *this;
	}
	VkSamplerCreateInfoBuilder& UnnormalisedCoord(bool value) noexcept
	{
		m_createInfo.unnormalizedCoordinates = value ? VK_TRUE : VK_FALSE;

		return *this;
	}

	[[nodiscard]]
	VkSamplerCreateInfo Get() const noexcept
	{
		return m_createInfo;
	}
	[[nodiscard]]
	const VkSamplerCreateInfo* GetPtr() const noexcept
	{
		return &m_createInfo;
	}

private:
	VkSamplerCreateInfo m_createInfo;

	static float s_maxAnisotropy;
};

class VKSampler
{
public:
	VKSampler(VkDevice device) : m_device{ device }, m_sampler{ VK_NULL_HANDLE } {}
	~VKSampler() noexcept;

	void CreateSampler(const VkSamplerCreateInfo* createInfo);
	void CreateSampler(const VkSamplerCreateInfo& createInfo);

	[[nodiscard]]
	VkSampler Get() const noexcept { return m_sampler; }

private:
	VkDevice  m_device;
	VkSampler m_sampler;

public:
	VKSampler(const VKSampler&) = delete;
	VKSampler& operator=(const VKSampler&) = delete;

	VKSampler(VKSampler&& other) noexcept : m_device{ other.m_device }, m_sampler{ other.m_sampler }
	{
		other.m_sampler = VK_NULL_HANDLE;
	}
	VKSampler& operator=(VKSampler&& other) noexcept
	{
		m_device        = other.m_device;
		m_sampler       = other.m_sampler;
		other.m_sampler = VK_NULL_HANDLE;

		return *this;
	}
};
#endif
