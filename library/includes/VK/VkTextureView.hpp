#ifndef VK_TEXTURE_VIEW_HPP_
#define VK_TEXTURE_VIEW_HPP_
#include <VkResources.hpp>
#include <utility>

// This is a wrapper around the Vulkan handle for VkImageView.
class VKImageView
{
public:
	VKImageView(VkDevice device)
		: m_device{ device }, m_imageView{ VK_NULL_HANDLE }, m_imageAspect{ 0u },
		m_mipBaseLevel{ 0u }, m_mipLevelCount{ 1u }
	{}
	~VKImageView() noexcept;

	void CreateView(
		VkImage image, VkFormat imageFormat, VkImageAspectFlags aspectFlags, VkImageViewType imageType,
		std::uint32_t mipBaseLevel = 0u, std::uint32_t mipLevelCount = 1u
	);

	void Destroy() noexcept;

	[[nodiscard]]
	VkImageView Get() const noexcept { return m_imageView; }
	[[nodiscard]]
	VkImageAspectFlags GetAspect() const noexcept { return m_imageAspect; }
	[[nodiscard]]
	std::uint32_t GetMipLevelCount() const noexcept { return m_mipLevelCount; }
	[[nodiscard]]
	std::uint32_t GetMipBaseLevel() const noexcept { return m_mipBaseLevel; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice           m_device;
	VkImageView        m_imageView;
	VkImageAspectFlags m_imageAspect;
	std::uint32_t      m_mipBaseLevel;
	std::uint32_t      m_mipLevelCount;

public:
	VKImageView(const VKImageView&) = delete;
	VKImageView& operator=(const VKImageView&) = delete;

	VKImageView(VKImageView&& other) noexcept
		: m_device{ other.m_device }, m_imageView{ std::exchange(other.m_imageView, VK_NULL_HANDLE) },
		m_imageAspect{ other.m_imageAspect }, m_mipBaseLevel{ other.m_mipBaseLevel },
		m_mipLevelCount{ other.m_mipLevelCount }
	{}
	VKImageView& operator=(VKImageView&& other) noexcept
	{
		SelfDestruct();

		m_device        = other.m_device;
		m_imageView     = std::exchange(other.m_imageView, VK_NULL_HANDLE);
		m_imageAspect   = other.m_imageAspect;
		m_mipBaseLevel  = other.m_mipBaseLevel;
		m_mipLevelCount = other.m_mipLevelCount;

		return *this;
	}
};

// This is can allocate a Texture resource and then wraps a VkImageView around it.
class VkTextureView
{
public:
	VkTextureView(VkDevice device, MemoryManager* memoryManager, VkMemoryPropertyFlagBits memoryType)
		: m_device{ device }, m_texture{ device, memoryManager, memoryType }, m_imageView{ device }
	{}

	void CreateView2D(
		std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		VkImageUsageFlags textureUsageFlags, VkImageAspectFlags aspectFlags,
		VkImageViewType imageType, const std::vector<std::uint32_t>& queueFamilyIndices,
		std::uint32_t mipBaseLevel = 0u, std::uint32_t mipLevelCount = 1u
	);
	void CreateView3D(
		std::uint32_t width, std::uint32_t height, std::uint32_t depth,
		VkFormat imageFormat, VkImageUsageFlags textureUsageFlags, VkImageAspectFlags aspectFlags,
		VkImageViewType imageType, const std::vector<std::uint32_t>& queueFamilyIndices,
		std::uint32_t mipBaseLevel = 0u, std::uint32_t mipLevelCount = 1u
	);

	void CreateView(
		Texture&& texture,
		VkFormat imageFormat, VkImageAspectFlags aspectFlags, VkImageViewType imageType,
		std::uint32_t mipBaseLevel = 0u, std::uint32_t mipLevelCount = 1u
	);

	void Destroy() noexcept;

	[[nodiscard]]
	const Texture& GetTexture() const noexcept { return m_texture; }
	[[nodiscard]]
	VkImageView GetView() const noexcept { return m_imageView.Get(); }
	[[nodiscard]]
	VkImageAspectFlags GetAspect() const noexcept { return m_imageView.GetAspect(); }
	[[nodiscard]]
	std::uint32_t GetMipLevelCount() const noexcept { return m_imageView.GetMipLevelCount(); }
	[[nodiscard]]
	std::uint32_t GetMipBaseLevel() const noexcept { return m_imageView.GetMipBaseLevel(); }

private:
	VkDevice    m_device;
	Texture     m_texture;
	VKImageView m_imageView;

public:
	VkTextureView(const VkTextureView&) = delete;
	VkTextureView& operator=(const VkTextureView&) = delete;

	VkTextureView(VkTextureView&& other) noexcept
		: m_device{ other.m_device }, m_texture{ std::move(other.m_texture) },
		m_imageView{ std::move(other.m_imageView) }
	{}
	VkTextureView& operator=(VkTextureView&& other) noexcept
	{
		m_device    = other.m_device;
		m_texture   = std::move(other.m_texture);
		m_imageView = std::move(other.m_imageView);

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

	void Create(const VkSamplerCreateInfo* createInfo);
	void Create(const VkSamplerCreateInfo& createInfo)
	{
		Create(&createInfo);
	}
	void Create(const VkSamplerCreateInfoBuilder& builder)
	{
		Create(builder.GetPtr());
	}

	[[nodiscard]]
	VkSampler Get() const noexcept { return m_sampler; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice  m_device;
	VkSampler m_sampler;

public:
	VKSampler(const VKSampler&) = delete;
	VKSampler& operator=(const VKSampler&) = delete;

	VKSampler(VKSampler&& other) noexcept
		: m_device{ other.m_device }, m_sampler{ std::exchange(other.m_sampler, VK_NULL_HANDLE) }
	{}
	VKSampler& operator=(VKSampler&& other) noexcept
	{
		SelfDestruct();

		m_device  = other.m_device;
		m_sampler = std::exchange(other.m_sampler, VK_NULL_HANDLE);

		return *this;
	}
};
#endif
