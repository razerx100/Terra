#ifndef VK_DESCRIPTOR_BUFFER_HPP_
#define VK_DESCRIPTOR_BUFFER_HPP_
#include <VkResources.hpp>
#include <VkTextureView.hpp>
#include <VkExtensionManager.hpp>
#include <array>

class DescriptorSetLayout
{
public:
	DescriptorSetLayout(VkDevice device)
		: m_device{ device }, m_layout{ VK_NULL_HANDLE }, m_layoutBindings{}, m_layoutBindingFlags{}
	{}
	~DescriptorSetLayout() noexcept;

	void Create();

	void AddBinding(
		std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
		VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags
	) noexcept;

	[[nodiscard]]
	VkDescriptorSetLayout Get() const noexcept { return m_layout; }

	void CleanUpTempData();

private:
	void SelfDestruct() noexcept;

private:
	VkDevice                                  m_device;
	VkDescriptorSetLayout                     m_layout;
	std::vector<VkDescriptorSetLayoutBinding> m_layoutBindings;
	std::vector<VkDescriptorBindingFlags>     m_layoutBindingFlags;

public:
	DescriptorSetLayout(const DescriptorSetLayout&) = delete;
	DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

	DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
		: m_device{ other.m_device }, m_layout{ other.m_layout },
		m_layoutBindings{ std::move(other.m_layoutBindings) },
		m_layoutBindingFlags{ std::move(other.m_layoutBindingFlags) }
	{
		other.m_layout = VK_NULL_HANDLE;
	}

	DescriptorSetLayout& operator=(DescriptorSetLayout&& other) noexcept
	{
		SelfDestruct();

		m_device             = other.m_device;
		m_layout             = other.m_layout;
		m_layoutBindings     = std::move(other.m_layoutBindings);
		m_layoutBindingFlags = std::move(other.m_layoutBindingFlags);

		other.m_layout = VK_NULL_HANDLE;

		return *this;
	}
};

class VkDescriptorBuffer
{
public:
	VkDescriptorBuffer(VkDevice device, MemoryManager* memoryManager)
		: m_device{ device }, m_setLayout{ device },
		m_descriptorBuffer{ device, memoryManager, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT },
		m_bufferFlags{ 0u },
		m_layoutOffset{ 0u } // This should be always 0u, as I have a single layout.
	{}

	VkDescriptorBuffer& AddBinding(
		std::uint32_t bindingIndex, VkDescriptorType type, std::uint32_t descriptorCount,
		VkShaderStageFlags shaderFlags, VkDescriptorBindingFlags bindingFlags = 0u
	) noexcept;

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
	VkDeviceSize const* GetLayoutOffset() const noexcept
	{
		return &m_layoutOffset;
	}
	[[nodiscard]]
	const DescriptorSetLayout& GetLayout() const noexcept
	{
		return m_setLayout;
	}

private:
	VkDevice            m_device;
	DescriptorSetLayout m_setLayout;
	Buffer              m_descriptorBuffer;
	VkBufferUsageFlags  m_bufferFlags;
	VkDeviceSize        m_layoutOffset;

	static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_descriptorInfo;
	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkExtDescriptorBuffer
	};

private:
	[[nodiscard]]
	VkDeviceSize GetBindingOffset(std::uint32_t binding) const noexcept;

	template<VkDescriptorType type>
	void GetBufferToDescriptor(
		const VkDescriptorAddressInfoEXT& bufferInfo, std::uint32_t bindingIndex
	) {
		using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

		VkDescriptorDataEXT descData{};
		// Will be needed when I add multiple descriptor support.
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

		// From what I understand, the call below will populate the cpu pointer with the specific
		// descriptor and then copy the data of getInfo to it. And possibly, once we have done that
		// we can just copy further getInfo s with different Buffer/Texture objects and just memcpy
		// them in. Or perhaps just memcpy the BufferObject pointer, don't have to do all these
		// GetInfoEXT stuffs anymore.
		DescBuffer::vkGetDescriptorEXT(
			m_device, &getInfo, descriptorSize,
			m_descriptorBuffer.CPUHandle() + GetBindingOffset(bindingIndex)
		);
	}

	[[nodiscard]]
	static VkDescriptorAddressInfoEXT GetBufferDescAddressInfo(
		VkDeviceAddress bufferStartingAddress, VkDeviceSize bufferSize, VkFormat texelBufferFormat
	) noexcept;

	template<VkDescriptorType type>
	void AddBufferToDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		const VkDeviceAddress bufferAddress = buffer.GpuPhysicalAddress() + bufferOffset;
		const VkDeviceSize actualBufferSize = bufferSize ? bufferSize : buffer.Size();

		const VkDescriptorAddressInfoEXT bufferDescAddressInfo = GetBufferDescAddressInfo(
			bufferAddress, actualBufferSize, VK_FORMAT_UNDEFINED
		);
		GetBufferToDescriptor<type>(bufferDescAddressInfo, bindingIndex);
	}

	template<VkDescriptorType type>
	void AddTexelBufferToDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, VkFormat texelBufferFormat,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		const VkDeviceAddress bufferAddress = buffer.GpuPhysicalAddress() + bufferOffset;
		const VkDeviceSize actualBufferSize = bufferSize ? bufferSize : buffer.Size();

		const VkDescriptorAddressInfoEXT bufferDescAddressInfo = GetBufferDescAddressInfo(
			bufferAddress, actualBufferSize, texelBufferFormat
		);
		GetBufferToDescriptor<type>(bufferDescAddressInfo, bindingIndex);
	}

	template<VkDescriptorType type>
	void AddImageToDescriptor(
		VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, std::uint32_t bindingIndex
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
			m_descriptorBuffer.CPUHandle() + GetBindingOffset(bindingIndex)
		);
	}

public:
	void AddStorageBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
			buffer, bindingIndex, bufferOffset, bufferSize
		);
	}
	void AddUniformBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
			buffer, bindingIndex, bufferOffset, bufferSize
		);
	}
	void AddUniformTexelBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, VkFormat texelFormat,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER>(
			buffer, bindingIndex, texelFormat, bufferOffset, bufferSize
		);
	}
	void AddStorageTexelBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, VkFormat texelFormat,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) {
		AddTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER>(
			buffer, bindingIndex, texelFormat, bufferOffset, bufferSize
		);
	}
	void AddCombinedImageDescriptor(
		const VkTextureView& textureView, const VKSampler& sampler, VkImageLayout imageLayout,
		std::uint32_t bindingIndex
	) {
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
			textureView.GetView(), sampler.Get(), imageLayout, bindingIndex
		);
	}
	void AddSampledImageDescriptor(
		const VkTextureView& textureView, VkImageLayout imageLayout, std::uint32_t bindingIndex
	) {
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(
			textureView.GetView(), VK_NULL_HANDLE, imageLayout, bindingIndex
		);
	}
	void AddStorageImageDescriptor(
		const VkTextureView& textureView, VkImageLayout imageLayout, std::uint32_t bindingIndex
	) {
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>(
			textureView.GetView(), VK_NULL_HANDLE, imageLayout, bindingIndex
		);
	}
	void AddSamplerDescriptor(const VKSampler& sampler, std::uint32_t bindingIndex)
	{
		AddImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLER>(
			VK_NULL_HANDLE, sampler.Get(), VK_IMAGE_LAYOUT_UNDEFINED, bindingIndex
		);
	}

	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{ return s_requiredExtensions; }

public:
	VkDescriptorBuffer(const VkDescriptorBuffer&) = delete;
	VkDescriptorBuffer& operator=(const VkDescriptorBuffer&) = delete;

	VkDescriptorBuffer(VkDescriptorBuffer&& other) noexcept
		: m_device{ other.m_device }, m_setLayout{ std::move(other.m_setLayout) },
		m_descriptorBuffer{ std::move(other.m_descriptorBuffer) },
		m_bufferFlags{ other.m_bufferFlags }, m_layoutOffset{ other.m_layoutOffset }
	{}

	VkDescriptorBuffer& operator=(VkDescriptorBuffer&& other) noexcept
	{
		m_device           = other.m_device;
		m_setLayout        = std::move(other.m_setLayout);
		m_descriptorBuffer = std::move(other.m_descriptorBuffer);
		m_bufferFlags      = other.m_bufferFlags;
		m_layoutOffset     = other.m_layoutOffset;

		return *this;
	}
};
#endif
