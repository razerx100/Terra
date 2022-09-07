#ifndef VK_RESOURCE_VIEWS_HPP_
#define VK_RESOURCE_VIEWS_HPP_
#include <VkResources.hpp>

class VkResourceView {
public:
	VkResourceView(VkDevice device) noexcept;

	void CreateResource(
		VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags,
		std::vector<std::uint32_t> queueFamilyIndices = {}
	);
	void BindBufferToMemory(
		VkDevice device, VkDeviceMemory memory, VkDeviceSize offset
	);
	void SetCPUWPtr(std::uint8_t* ptr) noexcept;

	[[nodiscard]]
	VkBuffer GetResource() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWPtr() const noexcept;

private:
	VkResource m_resource;
	std::uint8_t* m_cpuWPtr;
};
#endif
