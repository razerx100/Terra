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

enum class InstanceExtension
{
	VkExtDebugUtils,
	VkKhrSurface,
	VkKhrWin32Surface,
	VkKhrDisplay,
	VkKhrExternalMemoryCapabilities,
	None
};

template<typename ExtensionType>
class VkExtensionManager
{
public:
	VkExtensionManager() = default;

	template<class Derived>
	void AddExtension(this Derived& self, ExtensionType extension) noexcept
	{
		const auto extensionIndex = static_cast<size_t>(extension);

		self.m_extensions.set(extensionIndex);

		self.AddExtensionName(extensionIndex);
	}

	template<size_t Count, class Derived>
	void AddExtensions(
		this Derived& self, const std::array<ExtensionType, Count>& extensions
	) noexcept {
		for (const ExtensionType extension : extensions)
			self.AddExtension(extension);
	}

	template<class Derived>
	void AddExtensions(
		this Derived& self, const std::vector<ExtensionType>& extensions
	) noexcept {
		for (const ExtensionType extension : extensions)
			self.AddExtension(extension);
	}

	[[nodiscard]]
	bool IsExtensionActive(ExtensionType extension) const noexcept
	{
		return m_extensions.test(static_cast<size_t>(extension));
	}
	[[nodiscard]]
	const std::vector<const char*>& GetExtensionNames() const noexcept
	{
		return m_extensionNames;
	}
	[[nodiscard]]
	std::vector<ExtensionType> GetActiveExtensions() const noexcept
	{
		std::vector<ExtensionType> activeExtensions{};
		activeExtensions.reserve(std::size(m_extensionNames));

		const auto extensionCount = static_cast<size_t>(ExtensionType::None);

		for (size_t index = 0u; index < extensionCount; ++index)
			if (m_extensions.test(index))
				activeExtensions.emplace_back(static_cast<ExtensionType>(index));

		return activeExtensions;
	}

protected:
	std::bitset<static_cast<size_t>(ExtensionType::None)> m_extensions;
	std::vector<const char*>                              m_extensionNames;

public:
	VkExtensionManager(const VkExtensionManager&) = delete;
	VkExtensionManager& operator=(const VkExtensionManager&) = delete;

	VkExtensionManager(VkExtensionManager&& other) noexcept
		: m_extensions{ std::move(other.m_extensions) },
		m_extensionNames{ std::move(other.m_extensionNames) } {}

	VkExtensionManager& operator=(VkExtensionManager&& other) noexcept
	{
		m_extensions     = std::move(other.m_extensions);
		m_extensionNames = std::move(other.m_extensionNames);

		return *this;
	}
};

class VkDeviceExtensionManager : public VkExtensionManager<DeviceExtension>
{
	friend class VkExtensionManager<DeviceExtension>;

public:
	VkDeviceExtensionManager() = default;

	void PopulateExtensionFunctions(VkDevice device) const noexcept;

private:
	void AddExtensionName(size_t extensionIndex) noexcept;

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

public:
	VkDeviceExtensionManager(const VkDeviceExtensionManager&) = delete;
	VkDeviceExtensionManager& operator=(const VkDeviceExtensionManager&) = delete;

	VkDeviceExtensionManager(VkDeviceExtensionManager&& other) noexcept
		: VkExtensionManager{ std::move(other) }
	{}

	VkDeviceExtensionManager& operator=(VkDeviceExtensionManager&& other) noexcept
	{
		VkExtensionManager::operator=(std::move(other));

		return *this;
	}
};

class VkInstanceExtensionManager : public VkExtensionManager<InstanceExtension>
{
	friend class VkExtensionManager<InstanceExtension>;

public:
	VkInstanceExtensionManager() = default;

	void PopulateExtensionFunctions(VkInstance instance) const noexcept;

private:
	void AddExtensionName(size_t extensionIndex) noexcept;

private:
	template<typename T>
	static void PopulateFunctionPointer(
		VkInstance instance, const char* functionName, T& functionPtr
	) noexcept {
		functionPtr = reinterpret_cast<T>(vkGetInstanceProcAddr(instance, functionName));
	}

private:
	static void PopulateVkExtDebugUtils(VkInstance instance) noexcept;

public:
	VkInstanceExtensionManager(const VkInstanceExtensionManager&) = delete;
	VkInstanceExtensionManager& operator=(const VkInstanceExtensionManager&) = delete;

	VkInstanceExtensionManager(VkInstanceExtensionManager&& other) noexcept
		: VkExtensionManager{ std::move(other) }
	{}

	VkInstanceExtensionManager& operator=(VkInstanceExtensionManager&& other) noexcept
	{
		VkExtensionManager::operator=(std::move(other));

		return *this;
	}
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
		) {
			s_vkCmdDrawMeshTasksEXT(commandBuffer, groupCountX, groupCountY, groupCountZ);
		}
		static void vkCmdDrawMeshTasksIndirectCountEXT(
			VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
			VkDeviceSize countBufferOffset, std::uint32_t maxDrawCount, std::uint32_t stride
		) {
			s_vkCmdDrawMeshTasksIndirectCountEXT(
				commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride
			);
		}
		static void vkCmdDrawMeshTasksIndirectEXT(
			VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
			std::uint32_t drawCount, std::uint32_t stride
		) {
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
		) {
			s_vkCmdBindDescriptorBufferEmbeddedSamplersEXT(
				commandBuffer, pipelineBindPoint, layout, set
			);
		}
		static void vkCmdBindDescriptorBuffersEXT(
			VkCommandBuffer commandBuffer, std::uint32_t bufferCount,
			const VkDescriptorBufferBindingInfoEXT* pBindingInfos
		) {
			s_vkCmdBindDescriptorBuffersEXT(commandBuffer, bufferCount, pBindingInfos);
		}
		static void vkCmdSetDescriptorBufferOffsetsEXT(
			VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
			VkPipelineLayout layout, std::uint32_t firstSet, std::uint32_t setCount,
			const std::uint32_t* pBufferIndices, const VkDeviceSize* pOffsets
		) {
			s_vkCmdSetDescriptorBufferOffsetsEXT(
				commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pBufferIndices, pOffsets
			);
		}
		static void vkGetBufferOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkBufferCaptureDescriptorDataInfoEXT* pInfo, void* pData
		) {
			s_vkGetBufferOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetDescriptorEXT(
			VkDevice device, const VkDescriptorGetInfoEXT* pDescriptorInfo, size_t dataSize,
			void* pDescriptor
		) {
			s_vkGetDescriptorEXT(device, pDescriptorInfo, dataSize, pDescriptor);
		}
		static void vkGetDescriptorSetLayoutBindingOffsetEXT(
			VkDevice device, VkDescriptorSetLayout layout, std::uint32_t binding, VkDeviceSize* pOffset
		) {
			s_vkGetDescriptorSetLayoutBindingOffsetEXT(device, layout, binding, pOffset);
		}
		static void vkGetDescriptorSetLayoutSizeEXT(
			VkDevice device, VkDescriptorSetLayout layout, VkDeviceSize* pLayoutSizeInBytes
		) {
			s_vkGetDescriptorSetLayoutSizeEXT(device, layout, pLayoutSizeInBytes);
		}
		static void vkGetImageOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkImageCaptureDescriptorDataInfoEXT* pInfo, void* pData
		) {
			s_vkGetImageOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetImageViewOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkImageViewCaptureDescriptorDataInfoEXT* pInfo, void* pData
		) {
			s_vkGetImageViewOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetSamplerOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkSamplerCaptureDescriptorDataInfoEXT* pInfo, void* pData
		) {
			s_vkGetSamplerOpaqueCaptureDescriptorDataEXT(device, pInfo, pData);
		}
		static void vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT(
			VkDevice device, const VkAccelerationStructureCaptureDescriptorDataInfoEXT* pInfo,
			void* pData
		) {
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

namespace VkInstanceExtension
{
	class VkExtDebugUtils
	{
		friend class VkInstanceExtensionManager;
	public:
		static void vkCmdBeginDebugUtilsLabelEXT(
			VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo
		) {
			s_vkCmdBeginDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
		}
		static void vkCmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
		{
			s_vkCmdEndDebugUtilsLabelEXT(commandBuffer);
		}
		static void vkCmdInsertDebugUtilsLabelEXT(
			VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo
		) {
			s_vkCmdInsertDebugUtilsLabelEXT(commandBuffer, pLabelInfo);
		}
		static void vkCreateDebugUtilsMessengerEXT(
			VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pMessenger
		) {
			s_vkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
		}
		static void vkDestroyDebugUtilsMessengerEXT(
			VkInstance instance, VkDebugUtilsMessengerEXT messenger,
			const VkAllocationCallbacks* pAllocator
		) {
			s_vkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
		}
		static void vkQueueBeginDebugUtilsLabelEXT(
			VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo
		) {
			s_vkQueueBeginDebugUtilsLabelEXT(queue, pLabelInfo);
		}
		static void vkQueueEndDebugUtilsLabelEXT(VkQueue queue)
		{
			s_vkQueueEndDebugUtilsLabelEXT(queue);
		}
		static void vkQueueInsertDebugUtilsLabelEXT(
			VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo
		) {
			s_vkQueueInsertDebugUtilsLabelEXT(queue, pLabelInfo);
		}
		static void vkSetDebugUtilsObjectNameEXT(
			VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo
		) {
			s_vkSetDebugUtilsObjectNameEXT(device, pNameInfo);
		}
		static void vkSetDebugUtilsObjectTagEXT(
			VkDevice device, const VkDebugUtilsObjectTagInfoEXT* pTagInfo
		) {
			s_vkSetDebugUtilsObjectTagEXT(device, pTagInfo);
		}
		static void vkSubmitDebugUtilsMessageEXT(
			VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageTypes,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
		) {
			s_vkSubmitDebugUtilsMessageEXT(instance, messageSeverity, messageTypes, pCallbackData);
		}

	private:
		inline static PFN_vkCmdBeginDebugUtilsLabelEXT s_vkCmdBeginDebugUtilsLabelEXT       = nullptr;
		inline static PFN_vkCmdEndDebugUtilsLabelEXT s_vkCmdEndDebugUtilsLabelEXT           = nullptr;
		inline static PFN_vkCmdInsertDebugUtilsLabelEXT s_vkCmdInsertDebugUtilsLabelEXT     = nullptr;
		inline static PFN_vkCreateDebugUtilsMessengerEXT s_vkCreateDebugUtilsMessengerEXT   = nullptr;
		inline static PFN_vkDestroyDebugUtilsMessengerEXT s_vkDestroyDebugUtilsMessengerEXT = nullptr;
		inline static PFN_vkQueueBeginDebugUtilsLabelEXT s_vkQueueBeginDebugUtilsLabelEXT   = nullptr;
		inline static PFN_vkQueueEndDebugUtilsLabelEXT s_vkQueueEndDebugUtilsLabelEXT       = nullptr;
		inline static PFN_vkQueueInsertDebugUtilsLabelEXT s_vkQueueInsertDebugUtilsLabelEXT = nullptr;
		inline static PFN_vkSetDebugUtilsObjectNameEXT s_vkSetDebugUtilsObjectNameEXT       = nullptr;
		inline static PFN_vkSetDebugUtilsObjectTagEXT s_vkSetDebugUtilsObjectTagEXT         = nullptr;
		inline static PFN_vkSubmitDebugUtilsMessageEXT s_vkSubmitDebugUtilsMessageEXT       = nullptr;
	};
}
#endif
