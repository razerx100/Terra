#ifndef VK_EXTENSION_LOADER_HPP_
#define	VK_EXTENSION_LOADER_HPP_
#include <vulkan/vulkan.hpp>

enum class DeviceExtension
{
	VkExtMeshShader
};

class VkDeviceExtensionManager
{
public:
	void AddExtension(DeviceExtension extension) noexcept;
	void PopulateExtensionFunctions(VkDevice device) const noexcept;

	std::vector<const char*> GetExtensionNames() const noexcept;

private:
	template<typename T>
	static void PopulateFunctionPointer(
		VkDevice device, const char* functionName, T& functionPtr
	) noexcept {
		functionPtr = reinterpret_cast<T>(vkGetDeviceProcAddr(device, functionName));
	}

private:
	static void PopulateVkExtMeshShader(VkDevice device) noexcept;

private:
	std::vector<DeviceExtension> m_extentions;
};

namespace VkExtension
{
	class VkExtMeshShader
	{
		friend class VkDeviceExtensionManager;
	public:
		static void vkCmdDrawMeshTasksEXT(
			VkCommandBuffer commandBuffer,
			std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ
		)
		{
			s_vkCmdDrawMeshTasksEXT(commandBuffer, groupCountX, groupCountY, groupCountZ);
		}
		static void vkCmdDrawMeshTasksIndirectCountEXT(
			VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
			VkDeviceSize countBufferOffset, std::uint32_t maxDrawCount, std::uint32_t stride
		)
		{
			s_vkCmdDrawMeshTasksIndirectCountEXT(
				commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride
			);
		}
		static void vkCmdDrawMeshTasksIndirectEXT(
			VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
			std::uint32_t drawCount, std::uint32_t stride
		)
		{
			s_vkCmdDrawMeshTasksIndirectEXT(commandBuffer, buffer, offset, drawCount, stride);
		}

	private:
		inline static PFN_vkCmdDrawMeshTasksEXT s_vkCmdDrawMeshTasksEXT = nullptr;
		inline static PFN_vkCmdDrawMeshTasksIndirectCountEXT s_vkCmdDrawMeshTasksIndirectCountEXT = nullptr;
		inline static PFN_vkCmdDrawMeshTasksIndirectEXT s_vkCmdDrawMeshTasksIndirectEXT = nullptr;
	};
}
#endif
