#ifndef VK_EXTENSION_MANAGER_HPP_
#define	VK_EXTENSION_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <bitset>
#include <vector>
#include <array>

enum class DeviceExtension
{
	VkExtMeshShader,
	VkKhrSwapchain,
	VkExtMemoryBudget,
	VkExtDescriptorBuffer,
	None
};

class VkDeviceExtensionManager
{
public:
	void AddExtension(DeviceExtension extension) noexcept;

	template<size_t Count>
	void AddExtensions(const std::array<DeviceExtension, Count>& extensions) noexcept
	{
		for (const DeviceExtension extension : extensions)
			AddExtension(extension);
	}

	void PopulateExtensionFunctions(VkDevice device) const noexcept;

	[[nodiscard]]
	bool IsExtensionActive(DeviceExtension extension) const noexcept;
	[[nodiscard]]
	const std::vector<const char*>& GetExtensionNames() const noexcept;
	[[nodiscard]]
	std::vector<DeviceExtension> GetActiveExtensions() const noexcept;

private:
	template<typename T>
	static void PopulateFunctionPointer(
		VkDevice device, const char* functionName, T& functionPtr
	) noexcept {
		functionPtr = reinterpret_cast<T>(vkGetDeviceProcAddr(device, functionName));
	}

private:
	static void PopulateVkExtMeshShader(VkDevice device) noexcept;
	static void PopulateVkExtDescriptorBuffer(VkDevice device) noexcept;

private:
	std::bitset<static_cast<size_t>(DeviceExtension::None)> m_extensions;
	std::vector<const char*>                                m_extensionNames;
};

namespace VkDeviceExtension
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
		inline static PFN_vkCmdDrawMeshTasksEXT s_vkCmdDrawMeshTasksEXT                           = nullptr;
		inline static PFN_vkCmdDrawMeshTasksIndirectCountEXT s_vkCmdDrawMeshTasksIndirectCountEXT = nullptr;
		inline static PFN_vkCmdDrawMeshTasksIndirectEXT s_vkCmdDrawMeshTasksIndirectEXT           = nullptr;
	};

	class VkExtDescriptorBuffer
	{
		friend class VkDeviceExtensionManager;
	public:
		static void vkCmdBindDescriptorBufferEmbeddedSamplersEXT(
			VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
			VkPipelineLayout layout, std::uint32_t set
		)
		{
			s_vkCmdBindDescriptorBufferEmbeddedSamplersEXT(
				commandBuffer, pipelineBindPoint, layout, set
			);
		}
		static void vkCmdBindDescriptorBuffersEXT(
			VkCommandBuffer commandBuffer, std::uint32_t bufferCount,
			const VkDescriptorBufferBindingInfoEXT* pBindingInfos
		)
		{
			s_vkCmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);
		}
		static void vkCmdSetDescriptorBufferOffsetsEXT(
			VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
			VkPipelineLayout layout, std::uint32_t firstSet, std::uint32_t setCount,
			const std::uint32_t* pBufferIndices, const VkDeviceSize* pOffsets
		)
		{
			s_vkCmdSetDescriptorBufferOffsetsEXT(
				commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pBufferIndices, pOffsets
			);
		}
		static void vkGetBufferOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkBufferCaptureDescriptorDataInfoEXT* pInfo, void* pData
		)
		{
			s_vkGetBufferOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetDescriptorEXT(
			VkDevice device, const VkDescriptorGetInfoEXT* pDescriptorInfo, size_t dataSize,
			void* pDescriptor
		)
		{
			s_vkGetDescriptorEXT(device, pDescriptorInfo, dataSize, pDescriptor);
		}
		static void vkGetDescriptorSetLayoutBindingOffsetEXT(
			VkDevice device, VkDescriptorSetLayout layout, std::uint32_t binding, VkDeviceSize* pOffset
		)
		{
			s_vkGetDescriptorSetLayoutBindingOffsetEXT(device, layout, binding, pOffset);
		}
		static void vkGetDescriptorSetLayoutSizeEXT(
			VkDevice device, VkDescriptorSetLayout layout, VkDeviceSize* pLayoutSizeInBytes
		)
		{
			s_vkGetDescriptorSetLayoutSizeEXT(device, layout, pLayoutSizeInBytes);
		}
		static void vkGetImageOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkImageCaptureDescriptorDataInfoEXT* pInfo, void* pData
		)
		{
			s_vkGetImageOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetImageViewOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkImageViewCaptureDescriptorDataInfoEXT* pInfo, void* pData
		)
		{
			s_vkGetImageViewOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetSamplerOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkSamplerCaptureDescriptorDataInfoEXT* pInfo, void* pData
		)
		{
			s_vkGetSamplerOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkAccelerationStructureCaptureDescriptorDataInfoEXT* pInfo,
			void* pData
		)
		{
			s_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}

	private:
		inline static PFN_vkCmdBindDescriptorBufferEmbeddedSamplersEXT
			s_vkCmdBindDescriptorBufferEmbeddedSamplersEXT = nullptr;
		inline static PFN_vkCmdBindDescriptorBuffersEXT s_vkCmdBindDescriptorBuffersEXT = nullptr;
		inline static PFN_vkCmdSetDescriptorBufferOffsetsEXT s_vkCmdSetDescriptorBufferOffsetsEXT = nullptr;
		inline static PFN_vkGetBufferOpaqueCaptureDescriptorDataEXT
			s_vkGetBufferOpaqueCaptureDescriptorDataEXT = nullptr;
		inline static PFN_vkGetDescriptorEXT s_vkGetDescriptorEXT = nullptr;
		inline static PFN_vkGetDescriptorSetLayoutBindingOffsetEXT
			s_vkGetDescriptorSetLayoutBindingOffsetEXT = nullptr;
		inline static PFN_vkGetDescriptorSetLayoutSizeEXT s_vkGetDescriptorSetLayoutSizeEXT = nullptr;
		inline static PFN_vkGetImageOpaqueCaptureDescriptorDataEXT
			s_vkGetImageOpaqueCaptureDescriptorDataEXT = nullptr;
		inline static PFN_vkGetImageViewOpaqueCaptureDescriptorDataEXT
			s_vkGetImageViewOpaqueCaptureDescriptorDataEXT = nullptr;
		inline static PFN_vkGetSamplerOpaqueCaptureDescriptorDataEXT
			s_vkGetSamplerOpaqueCaptureDescriptorDataEXT = nullptr;
		inline static PFN_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT
			s_vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT = nullptr;
	};
}
#endif
