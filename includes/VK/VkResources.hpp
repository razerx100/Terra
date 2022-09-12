#ifndef VK_RESOURCES_HPP_
#define VK_RESOURCES_HPP_
#include <vulkan/vulkan.hpp>
#include <vector>

template<typename ResourceType>
class VkBaseResource {
public:
	[[nodiscard]]
	ResourceType GetResource() const noexcept {
		return m_resource;
	}

protected:
	VkBaseResource() noexcept : m_resource{ VK_NULL_HANDLE } {}
	VkBaseResource(ResourceType resource) noexcept : m_resource{ resource } {}

	template<typename CreateInfo>
	static void ConfigureBufferQueueAccess(
		const std::vector<std::uint32_t>& queueFamilyIndices, CreateInfo& bufferInfo
	) noexcept {
		std::uint32_t queueIndicesSize =
			static_cast<std::uint32_t>(std::size(queueFamilyIndices));

		if (queueIndicesSize > 1u) {
			bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
			bufferInfo.queueFamilyIndexCount = queueIndicesSize;
			bufferInfo.pQueueFamilyIndices = std::data(queueFamilyIndices);
		}
		else
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

protected:
	ResourceType m_resource;
};

class VkResource : public VkBaseResource<VkBuffer> {
public:
	VkResource(VkDevice device) noexcept;
	~VkResource() noexcept;

	VkResource(const VkResource&) = delete;
	VkResource& operator=(const VkResource&) = delete;

	VkResource(VkResource&& resource) noexcept;
	VkResource& operator=(VkResource&& resource) noexcept;

	void CreateResource(
		VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
		const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void CleanUpResource() noexcept;

	[[nodiscard]]
	VkMemoryRequirements GetMemoryRequirements(VkDevice device) const noexcept {
		VkMemoryRequirements memReq{};
		vkGetBufferMemoryRequirements(device, m_resource, &memReq);

		return memReq;
	}

private:
	VkDevice m_deviceRef;
};

class VkImageResource : public VkBaseResource<VkImage> {
public:
	VkImageResource(VkDevice device) noexcept;
	~VkImageResource() noexcept;

	VkImageResource(const VkImageResource&) = delete;
	VkImageResource& operator=(const VkImageResource&) = delete;

	VkImageResource(VkImageResource&& resource) noexcept;
	VkImageResource& operator=(VkImageResource&& resource) noexcept;

	void CreateResource(
		VkDevice device, std::uint32_t width, std::uint32_t height, VkFormat imageFormat,
		VkImageUsageFlags usageFlags, const std::vector<std::uint32_t>& queueFamilyIndices
	);
	void CleanUpResource() noexcept;

	[[nodiscard]]
	VkMemoryRequirements GetMemoryRequirements(VkDevice device) const noexcept {
		VkMemoryRequirements memReq{};
		vkGetImageMemoryRequirements(device, m_resource, &memReq);

		return memReq;
	}

private:
	VkDevice m_deviceRef;
};
#endif
