#ifndef VK_DESCRIPTOR_BUFFER_HPP_
#define VK_DESCRIPTOR_BUFFER_HPP_
#include <VkResources.hpp>
#include <VkDescriptorSetLayout.hpp>
#include <VkTextureView.hpp>
#include <VkExtensionManager.hpp>
#include <VkCommandQueue.hpp>
#include <VkPipelineLayout.hpp>
#include <array>
#include <vector>

namespace Terra
{
class VkDescriptorBuffer
{
public:
	VkDescriptorBuffer(
		VkDevice device, MemoryManager* memoryManager, std::uint32_t setLayoutCount
	);

	VkDescriptorBuffer& AddBinding(
		std::uint32_t bindingIndex, size_t setLayoutIndex, VkDescriptorType type,
		std::uint32_t descriptorCount, VkShaderStageFlags shaderFlags,
		VkDescriptorBindingFlags bindingFlags = 0u
	) noexcept;

	void CreateBuffer(const std::vector<std::uint32_t>& queueFamilyIndices = {});

	// This will recreate the Descriptor Buffer as well and retain all the old descriptors.
	void RecreateSetLayout(
		size_t setLayoutIndex, const std::vector<VkDescriptorSetLayoutBinding>& oldSetLayoutBindings,
		const std::vector<std::uint32_t>& queueFamilyIndices = {}
	);

	static void SetDescriptorBufferInfo(VkPhysicalDevice physicalDevice) noexcept;

	[[nodiscard]]
	std::vector<VkDescriptorSetLayout> GetValidLayouts() const noexcept;
	[[nodiscard]]
	const DescriptorSetLayout& GetLayout(size_t index) const noexcept
	{
		return m_setLayouts[index];
	}
	[[nodiscard]]
	bool IsCreated() const noexcept { return m_descriptorBuffer.Get() != VK_NULL_HANDLE; }

	static void BindDescriptorBuffer(
		const VkDescriptorBuffer& descriptorBuffer, const VKCommandBuffer& cmdBuffer,
		VkPipelineBindPoint bindPoint, const PipelineLayout& pipelineLayout
	);

private:
	[[nodiscard]]
	VkDeviceAddress GpuPhysicalAddress() const noexcept
	{
		return m_descriptorBuffer.GpuPhysicalAddress();
	}
	[[nodiscard]]
	static constexpr VkBufferUsageFlags GetFlags() noexcept
	{
		return VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
	}

	void _createBuffer(
		Buffer& descriptorBuffer, const std::vector<std::uint32_t>& queueFamilyIndices
	);

private:
	VkDevice                         m_device;
	MemoryManager*                   m_memoryManager;
	std::vector<DescriptorSetLayout> m_setLayouts;
	std::vector<std::uint32_t>       m_bufferIndices;
	std::vector<VkDeviceSize>        m_validOffsets;
	std::vector<VkDeviceSize>        m_layoutOffsets;
	Buffer                           m_descriptorBuffer;

	static VkPhysicalDeviceDescriptorBufferPropertiesEXT s_descriptorInfo;
	static constexpr std::array s_requiredExtensions
	{
		DeviceExtension::VkExtDescriptorBuffer
	};

public:
	template<VkDescriptorType type>
	[[nodiscard]]
	static size_t GetDescriptorSize() noexcept
	{
		size_t descriptorSize = 0u;

		if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			descriptorSize = s_descriptorInfo.storageBufferDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			descriptorSize = s_descriptorInfo.uniformBufferDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
			descriptorSize = s_descriptorInfo.uniformTexelBufferDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
			descriptorSize = s_descriptorInfo.storageTexelBufferDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			descriptorSize = s_descriptorInfo.combinedImageSamplerDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			descriptorSize = s_descriptorInfo.storageImageDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			descriptorSize = s_descriptorInfo.sampledImageDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			descriptorSize = s_descriptorInfo.samplerDescriptorSize;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
			descriptorSize = s_descriptorInfo.inputAttachmentDescriptorSize;

		return descriptorSize;
	}

	[[nodiscard]]
	static size_t GetDescriptorSize(VkDescriptorType type) noexcept
	{
		size_t descriptorSize = 0u;

		if (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			descriptorSize = s_descriptorInfo.storageBufferDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			descriptorSize = s_descriptorInfo.uniformBufferDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
			descriptorSize = s_descriptorInfo.uniformTexelBufferDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
			descriptorSize = s_descriptorInfo.storageTexelBufferDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			descriptorSize = s_descriptorInfo.combinedImageSamplerDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			descriptorSize = s_descriptorInfo.storageImageDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			descriptorSize = s_descriptorInfo.sampledImageDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			descriptorSize = s_descriptorInfo.samplerDescriptorSize;
		else if (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
			descriptorSize = s_descriptorInfo.inputAttachmentDescriptorSize;

		return descriptorSize;
	}

private:
	[[nodiscard]]
	VkDeviceSize GetBindingOffset(std::uint32_t binding, size_t setLayoutIndex) const noexcept;

	[[nodiscard]]
	VkDeviceSize GetDescriptorOffset(
		std::uint32_t bindingIndex, size_t setLayoutIndex, size_t descriptorSize,
		std::uint32_t descriptorIndex
	) const noexcept;

	[[nodiscard]]
	void* GetDescriptorAddress(
		std::uint32_t bindingIndex, size_t setLayoutIndex, size_t descriptorSize,
		std::uint32_t descriptorIndex
	) const noexcept;

	template<VkDescriptorType type>
	[[nodiscard]]
	static VkDescriptorDataEXT GetDescriptorData(const VkDescriptorAddressInfoEXT& bufferInfo) noexcept
	{
		VkDescriptorDataEXT descData{};

		if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
			descData.pStorageBuffer = &bufferInfo;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			descData.pUniformBuffer = &bufferInfo;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
			descData.pUniformTexelBuffer = &bufferInfo;
		else if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)
			descData.pStorageTexelBuffer = &bufferInfo;

		return descData;
	}

	template<VkDescriptorType type>
	[[nodiscard]]
	static VkDescriptorDataEXT GetDescriptorData(
		VkImageView imageView, const VkSampler& sampler, VkDescriptorImageInfo& imageInfo
	) noexcept {
		VkDescriptorDataEXT descData{};

		if constexpr (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			imageInfo.sampler   = sampler;
			imageInfo.imageView = imageView;

			descData.pCombinedImageSampler = &imageInfo;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			imageInfo.imageView = imageView;

			descData.pStorageImage = &imageInfo;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		{
			imageInfo.imageView = imageView;

			descData.pSampledImage = &imageInfo;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT)
		{
			imageInfo.imageView = imageView;

			descData.pInputAttachmentImage = &imageInfo;
		}
		else if constexpr (type == VK_DESCRIPTOR_TYPE_SAMPLER)
			descData.pSampler = &sampler;

		return descData;
	}

	[[nodiscard]]
	static VkDescriptorAddressInfoEXT GetBufferDescAddressInfo(
		VkDeviceAddress bufferStartingAddress, VkDeviceSize bufferSize, VkFormat texelBufferFormat
	) noexcept;

public:
	template<VkDescriptorType type>
	[[nodiscard]]
	void const* GetDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex,
		const VkDescriptorDataEXT& descData
	) const {
		const VkDescriptorGetInfoEXT getInfo
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT,
			.type  = type,
			.data  = descData
		};

		const size_t descriptorSize = GetDescriptorSize<type>();

		void* descriptorAddress = GetDescriptorAddress(
			bindingIndex, setLayoutIndex, descriptorSize, descriptorIndex
		);

		using DescBuffer = VkDeviceExtension::VkExtDescriptorBuffer;

		DescBuffer::vkGetDescriptorEXT(m_device, &getInfo, descriptorSize, descriptorAddress);

		return descriptorAddress;
	}

	template<VkDescriptorType type>
	[[nodiscard]]
	void const* GetDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		const size_t descriptorSize = GetDescriptorSize<type>();

		return GetDescriptorAddress(bindingIndex, setLayoutIndex, descriptorSize, descriptorIndex);
	}

	template<VkDescriptorType type>
	void SetDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		const size_t descriptorSize = GetDescriptorSize<type>();

		memcpy(
			GetDescriptorAddress(bindingIndex, setLayoutIndex, descriptorSize, descriptorIndex),
			descriptorAddress, descriptorSize
		);
	}

private:
	template<VkDescriptorType type>
	[[nodiscard]]
	void const* GetBufferToDescriptor(
		const VkDescriptorAddressInfoEXT& bufferInfo, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		const VkDescriptorDataEXT descData = GetDescriptorData<type>(bufferInfo);

		return GetDescriptor<type>(bindingIndex, setLayoutIndex, descriptorIndex, descData);
	}

public:
	template<VkDescriptorType type>
	void const* GetBufferToDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		const VkDeviceAddress bufferAddress = buffer.GpuPhysicalAddress() + bufferOffset;
		const VkDeviceSize actualBufferSize = bufferSize ? bufferSize : buffer.BufferSize();

		const VkDescriptorAddressInfoEXT bufferDescAddressInfo = GetBufferDescAddressInfo(
			bufferAddress, actualBufferSize, VK_FORMAT_UNDEFINED
		);

		return GetBufferToDescriptor<type>(
			bufferDescAddressInfo, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}

	template<VkDescriptorType type>
	void const* GetTexelBufferToDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		VkFormat texelBufferFormat, std::uint32_t descriptorIndex, VkDeviceAddress bufferOffset = 0u,
		VkDeviceSize bufferSize = 0u
	) const {
		const VkDeviceAddress bufferAddress = buffer.GpuPhysicalAddress() + bufferOffset;
		const VkDeviceSize actualBufferSize = bufferSize ? bufferSize : buffer.BufferSize();

		const VkDescriptorAddressInfoEXT bufferDescAddressInfo = GetBufferDescAddressInfo(
			bufferAddress, actualBufferSize, texelBufferFormat
		);

		return GetBufferToDescriptor<type>(
			bufferDescAddressInfo, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}

	template<VkDescriptorType type>
	void const* GetImageToDescriptor(
		VkImageView imageView, VkSampler sampler, VkImageLayout imageLayout, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		VkDescriptorImageInfo imageInfo{ .imageLayout = imageLayout };

		const VkDescriptorDataEXT descData = GetDescriptorData<type>(imageView, sampler, imageInfo);

		return GetDescriptor<type>(bindingIndex, setLayoutIndex, descriptorIndex, descData);
	}

public:
	[[nodiscard]]
	void const* GetStorageBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		return GetBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
			buffer, bindingIndex, setLayoutIndex, descriptorIndex, bufferOffset, bufferSize
		);
	}
	[[nodiscard]]
	void const* GetUniformBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		return GetBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
			buffer, bindingIndex, setLayoutIndex, descriptorIndex, bufferOffset, bufferSize
		);
	}
	[[nodiscard]]
	void const* GetUniformTexelBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkFormat texelFormat, VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		return GetTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER>(
			buffer, bindingIndex, setLayoutIndex, texelFormat, descriptorIndex, bufferOffset, bufferSize
		);
	}
	[[nodiscard]]
	void const* GetStorageTexelBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkFormat texelFormat, VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		return GetTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER>(
			buffer, bindingIndex, setLayoutIndex, texelFormat, descriptorIndex, bufferOffset, bufferSize
		);
	}
	[[nodiscard]]
	void const* GetCombinedImageDescriptor(
		const VkTextureView& textureView, const VKSampler& sampler, VkImageLayout imageLayout,
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		return GetImageToDescriptor<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
			textureView.GetVkView(), sampler.Get(), imageLayout, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetSampledImageDescriptor(
		const VkTextureView& textureView, VkImageLayout imageLayout, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		return GetImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(
			textureView.GetVkView(), VK_NULL_HANDLE, imageLayout, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetInputAttachmentDescriptor(
		const VkTextureView& textureView, VkImageLayout imageLayout, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		return GetImageToDescriptor<VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT>(
			textureView.GetVkView(), VK_NULL_HANDLE, imageLayout, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetStorageImageDescriptor(
		const VkTextureView& textureView, VkImageLayout imageLayout, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		return GetImageToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>(
			textureView.GetVkView(), VK_NULL_HANDLE, imageLayout, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetSamplerDescriptor(
		const VKSampler& sampler, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const {
		return GetImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLER>(
			VK_NULL_HANDLE, sampler.Get(), VK_IMAGE_LAYOUT_UNDEFINED, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetStorageBufferDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetUniformBufferDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetUniformTexelBufferDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetStorageTexelBufferDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetCombinedImageDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetSampledImageDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetStorageImageDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	[[nodiscard]]
	void const* GetSamplerDescriptor(
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const noexcept {
		return GetDescriptor<VK_DESCRIPTOR_TYPE_SAMPLER>(
			bindingIndex, setLayoutIndex, descriptorIndex
		);
	}

	void SetStorageBufferDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetUniformBufferDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetUniformTexelBufferDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetStorageTexelBufferDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetCombinedImageDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetSampledImageDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetInputAttachmentDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetStorageImageDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}
	void SetSamplerDescriptor(
		void const* descriptorAddress, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const noexcept {
		SetDescriptor<VK_DESCRIPTOR_TYPE_SAMPLER>(
			descriptorAddress, bindingIndex, setLayoutIndex, descriptorIndex
		);
	}

	void SetStorageBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		GetBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_BUFFER>(
			buffer, bindingIndex, setLayoutIndex, descriptorIndex, bufferOffset, bufferSize
		);
	}
	void SetUniformBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		GetBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER>(
			buffer, bindingIndex, setLayoutIndex, descriptorIndex, bufferOffset, bufferSize
		);
	}
	void SetUniformTexelBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkFormat texelFormat, VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		GetTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER>(
			buffer, bindingIndex, setLayoutIndex, texelFormat, descriptorIndex, bufferOffset, bufferSize
		);
	}
	void SetStorageTexelBufferDescriptor(
		const Buffer& buffer, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex,
		VkFormat texelFormat, VkDeviceAddress bufferOffset = 0u, VkDeviceSize bufferSize = 0u
	) const {
		GetTexelBufferToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER>(
			buffer, bindingIndex, setLayoutIndex,
			texelFormat, descriptorIndex, bufferOffset, bufferSize
		);
	}
	void SetCombinedImageDescriptor(
		const VkTextureView& textureView, const VKSampler& sampler, VkImageLayout imageLayout,
		std::uint32_t bindingIndex, size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		GetImageToDescriptor<VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER>(
			textureView.GetVkView(), sampler.Get(), imageLayout, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	void SetSampledImageDescriptor(
		const VkTextureView& textureView, VkImageLayout imageLayout, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		GetImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE>(
			textureView.GetVkView(), VK_NULL_HANDLE, imageLayout, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	void SetStorageImageDescriptor(
		const VkTextureView& textureView, VkImageLayout imageLayout, std::uint32_t bindingIndex,
		size_t setLayoutIndex, std::uint32_t descriptorIndex
	) const {
		GetImageToDescriptor<VK_DESCRIPTOR_TYPE_STORAGE_IMAGE>(
			textureView.GetVkView(), VK_NULL_HANDLE, imageLayout, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}
	void SetSamplerDescriptor(
		const VKSampler& sampler, std::uint32_t bindingIndex, size_t setLayoutIndex,
		std::uint32_t descriptorIndex
	) const {
		GetImageToDescriptor<VK_DESCRIPTOR_TYPE_SAMPLER>(
			VK_NULL_HANDLE, sampler.Get(), VK_IMAGE_LAYOUT_UNDEFINED, bindingIndex, setLayoutIndex,
			descriptorIndex
		);
	}

	[[nodiscard]]
	static const decltype(s_requiredExtensions)& GetRequiredExtensions() noexcept
	{
		return s_requiredExtensions;
	}

public:
	VkDescriptorBuffer(const VkDescriptorBuffer&) = delete;
	VkDescriptorBuffer& operator=(const VkDescriptorBuffer&) = delete;

	VkDescriptorBuffer(VkDescriptorBuffer&& other) noexcept
		: m_device{ other.m_device }, m_memoryManager{ other.m_memoryManager },
		m_setLayouts{ std::move(other.m_setLayouts) },
		m_bufferIndices{ std::move(other.m_bufferIndices) },
		m_validOffsets{ std::move(other.m_validOffsets) },
		m_layoutOffsets{ std::move(other.m_layoutOffsets) },
		m_descriptorBuffer{ std::move(other.m_descriptorBuffer) }
	{}

	VkDescriptorBuffer& operator=(VkDescriptorBuffer&& other) noexcept
	{
		m_device           = other.m_device;
		m_memoryManager    = other.m_memoryManager;
		m_setLayouts       = std::move(other.m_setLayouts);
		m_bufferIndices    = std::move(other.m_bufferIndices);
		m_validOffsets     = std::move(other.m_validOffsets);
		m_layoutOffsets    = std::move(other.m_layoutOffsets);
		m_descriptorBuffer = std::move(other.m_descriptorBuffer);

		return *this;
	}
};
}
#endif
