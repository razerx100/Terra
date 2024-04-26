#ifndef VK_DESCRIPTOR_BUFFER_HPP_
#define VK_DESCRIPTOR_BUFFER_HPP_
#include <VkResources.hpp>
#include <VkExtensionManager.hpp>
#include <array>

class DescriptorSetLayout
{
public:
	DescriptorSetLayout(VkDevice device) : m_device{ device }, m_layout{ VK_NULL_HANDLE } {}
	~DescriptorSetLayout() noexcept;

	void CreateLayout(
		std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
		VkShaderStageFlags shaderFlags
	);

	[[nodiscard]]
	VkDescriptorSetLayout Get() const noexcept { return m_layout; }

private:
	void SelfDestruct() noexcept;

private:
	VkDevice              m_device;
	VkDescriptorSetLayout m_layout;

public:
	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

	DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
		: m_device{ other.m_device }, m_layout{ other.m_layout }
	{
		other.m_layout = VK_NULL_HANDLE;
	}

	DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept
	{
		SelfDestruct();

		m_device       = other.m_device;
		m_layout       = other.m_layout;
		other.m_layout = VK_NULL_HANDLE;

		return *this;
	}
};

class VkDescriptorBuffer
{
public:
	VkDescriptorBuffer(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_setLayouts{},
		m_descriptorBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_bufferFlags{ 0u }, m_layoutOffsets{}
	{}

	VkDescriptorBuffer& AddLayout(
		std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
		VkShaderStageFlags shaderFlags
	);

	void CreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices = {});

	static void SetDescriptorBufferInfo(VkPhysicalDevice physicalDevice) noexcept;

	[[nodiscard]]
	VkDeviceAddress GpuPhysicalAddress() const noexcept
	{
		return m_descriptorBuffer.GpuPhysicalAddress();
	}
	[[nodiscard]]
	VkBufferUsageFlags GetFlags() const noexcept
	{
		return m_bufferFlags;
	}
	[[nodiscard]]
	const std::vector<VkDeviceSize>& GetLayoutOffsets() const noexcept
	{
		return m_layoutOffsets;
	}
	[[nodiscard]]
	const std::vector<DescriptorSetLayout>& GetLayouts() const noexcept
	{
		return m_setLayouts;
	}

private:
	VkDevice                         m_device;
	std::vector<DescriptorSetLayout> m_setLayouts;
	Buffer                           m_descriptorBuffer;
	VkBufferUsageFlags               m_bufferFlags;
	std::vector<VkDeviceSize>        m_layoutOffsets;

	static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_descriptorInfo;
	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkExtDescriptorBuffer
	};

private:
	template<VkDescriptorType type>
	void GetBufferToDescriptor(
		const VkDescriptorAddressInfoEXT& bufferInfo, size_t layoutIndex
	) {
		using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

		VkDescriptorDataEXT descData{};
		size_t descriptorSize = 0u;

		if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			descData.pStorageBuffer = &bufferInfo;
			descriptorSize          = s_descriptorInfo.storageBufferDescriptorSize;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			descData.pUniformBuffer = &bufferInfo;
			descriptorSize          = s_descriptorInfo.uniformBufferDescriptorSize;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
		{
			descData.pUniformTexelBuffer = &bufferInfo;
			descriptorSize               = s_descriptorInfo.uniformTexelBufferDescriptorSize;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
		{
			descData.pStorageTexelBuffer = &bufferInfo;
			descriptorSize               = s_descriptorInfo.storageTexelBufferDescriptorSize;
		}

		VkDescriptorGetInfoEXT getInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type  = type,
			.data  = descData
		};

		// It might be possible to do this with a memcpy once it has been fetched with
		// vkGetDescriptorEXT once. But I am not sure how that would work, so keeping it as is, for now.
		DescBuffer::vkGetDescriptorEXT(
			m_device, &getInfo, descriptorSize,
			m_descriptorBuffer.CPUHandle() + m_layoutOffsets.at(layoutIndex) + descriptorSize
		);
	}

	[[nodiscard]]
	static VkDescriptorAddressInfoEXT GetBufferDescAddressInfo(
		VkDeviceAddress bufferStartingAddress, VkDeviceSize bufferSize, VkFormat texelBufferFormat
	) noexcept;

	template<VkDescriptorType type>
	void AddBufferToDescriptor(
		const Buffer& buffer, size_t layoutIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		const VkDeviceAddress bufferAddress = buffer.GpuPhysicalAddress() + bufferOffset;
		const VkDeviceSize actualBufferSize = bufferSize ? bufferSize : buffer.Size();

		const VkDescriptorAddressInfoEXT bufferDescAddressInfo = GetBufferDescAddressInfo(
			bufferAddress, actualBufferSize, VK_FORMAT_UNDEFINED
		);
		GetBufferToDescriptor<type>(bufferDescAddressInfo, layoutIndex);
	}

	template<VkDescriptorType type>
	void AddTexelBufferToDescriptor(
		const Buffer& buffer, size_t layoutIndex, VkFormat texelBufferFormat,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		const VkDeviceAddress bufferAddress = buffer.GpuPhysicalAddress() + bufferOffset;
		const VkDeviceSize actualBufferSize = bufferSize ? bufferSize : buffer.Size();

		const VkDescriptorAddressInfoEXT bufferDescAddressInfo = GetBufferDescAddressInfo(
			bufferAddress, actualBufferSize, texelBufferFormat
		);
		GetBufferToDescriptor<type>(bufferDescAddressInfo, layoutIndex);
	}

	template<VkDescriptorType type>
	void AddImageToDescriptor(
		VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, size_t layoutIndex
	) {
		using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

		VkDescriptorImageInfo imageInfo{ .imageLayout = imageLayout };

		VkDescriptorDataEXT descData{};
		size_t descriptorSize = 0u;

		if constexpr (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			imageInfo.sampler              = sampler;
			imageInfo.imageView            = imageView;

			descData.pCombinedImageSampler = &imageInfo;
			descriptorSize                 = s_descriptorInfo.combinedImageSamplerDescriptorSize;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			imageInfo.imageView    = imageView;

			descData.pStorageImage = &imageInfo;
			descriptorSize         = s_descriptorInfo.storageImageDescriptorSize;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		{
			imageInfo.imageView    = imageView;

			descData.pSampledImage = &imageInfo;
			descriptorSize         = s_descriptorInfo.sampledImageDescriptorSize;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
		{
			descData.pSampler = &sampler;
			descriptorSize    = s_descriptorInfo.samplerDescriptorSize;
		}

		VkDescriptorGetInfoEXT getInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type  = type,
			.data  = descData
		};

		DescBuffer::vkGetDescriptorEXT(
			m_device, &getInfo, descriptorSize,
			m_descriptorBuffer.CPUHandle() + m_layoutOffsets.at(layoutIndex) + descriptorSize
		);
	}

public:
	void AddStorageBufferDescriptor(
		const Buffer& buffer, size_t layoutIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
			buffer, layoutIndex, bufferOffset, bufferSize
		);
	}
	void AddUniformBufferDescriptor(
		const Buffer& buffer, size_t layoutIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
			buffer, layoutIndex, bufferOffset, bufferSize
		);
	}
	void AddUniformTexelBufferDescriptor(
		const Buffer& buffer, size_t layoutIndex, VkFormat texelFormat,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER>(
			buffer, layoutIndex, texelFormat, bufferOffset, bufferSize
		);
	}
	void AddStorageTexelBufferDescriptor(
		const Buffer& buffer, size_t layoutIndex, VkFormat texelFormat,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER>(
			buffer, layoutIndex, texelFormat, bufferOffset, bufferSize
		);
	}
	void AddCombinedImageDescriptor(
		VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, size_t layoutIndex
	)
	{
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
			imageView, sampler, imageLayout, layoutIndex
		);
	}
	void AddSampledImageDescriptor(
		VkImageView imageView, VkImageLayout imageLayout, size_t layoutIndex
	)
	{
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(
			imageView, VK_NULL_HANDLE, imageLayout, layoutIndex
		);
	}
	void AddStorageImageDescriptor(
		VkImageView imageView, VkImageLayout imageLayout, size_t layoutIndex
	)
	{
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>(
			imageView, VK_NULL_HANDLE, imageLayout, layoutIndex
		);
	}
	void AddSamplerDescriptor(VkSampler sampler, size_t layoutIndex)
	{
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLER>(
			VK_NULL_HANDLE, sampler, VK_IMAGE_LAYOUT_UNDEFINED, layoutIndex
		);
	}

	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }

public:
	VkDescriptorBuffer(const VkDescriptorBuffer&) = delete;
	VkDescriptorBuffer& operator=(const VkDescriptorBuffer&) = delete;

	VkDescriptorBuffer(VkDescriptorBuffer&& other) noexcept
		: m_device{ other.m_device }, m_setLayouts{ std::move(other.m_setLayouts) },
		m_descriptorBuffer{ std::move(other.m_descriptorBuffer) },
		m_bufferFlags{ other.m_bufferFlags }, m_layoutOffsets{ std::move(other.m_layoutOffsets) }
	{}

	VkDescriptorBuffer& operator=(VkDescriptorBuffer&& other) noexcept
	{
		m_device           = other.m_device;
		m_setLayouts       = std::move(other.m_setLayouts);
		m_descriptorBuffer = std::move(other.m_descriptorBuffer);
		m_bufferFlags      = other.m_bufferFlags;
		m_layoutOffsets    = std::move(other.m_layoutOffsets);

		return *this;
	}
};
#endif
