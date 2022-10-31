#ifndef BUFFER_MANAGER_HPP_
#define BUFFER_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <VkResourceViews.hpp>
#include <DescriptorSetManager.hpp>

#include <IModel.hpp>

struct ModelConstantBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	DirectX::XMFLOAT3 modelOffset;
};

class BufferManager {
public:
	BufferManager(
		VkDevice device, std::vector<std::uint32_t> queueFamilyIndices,
		std::uint32_t bufferCount
	) noexcept;

	void Update(VkDeviceSize index) const noexcept;
	void BindVertexBuffer(VkCommandBuffer commandBuffer) const noexcept;

	void CreateBuffers(VkDevice device) noexcept;
	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void AddModelInputs(
		VkDevice device,
		std::unique_ptr<std::uint8_t> vertices, size_t vertexBufferSize,
		std::unique_ptr<std::uint8_t> indices, size_t indexBufferSize
	);
	void BindResourceToMemory(VkDevice device);
	void RecordCopy(VkCommandBuffer copyCmdBuffer) noexcept;
	void ReleaseUploadResources() noexcept;

private:
	void UpdateModelData(VkDeviceSize index) const noexcept;

	static void AddDescriptorForBuffer(
		const VkResourceView& buffer, std::uint32_t bufferCount, std::uint32_t bindingSlot,
		VkDescriptorType descriptorType, VkShaderStageFlagBits shaderStage,
		DescriptorSetManager* const descriptorSetManager
	) noexcept;

private:
	VkResourceView m_cameraBuffer;
	VkUploadableBufferResourceView m_gVertexBuffer;
	VkUploadableBufferResourceView m_gIndexBuffer;
	std::vector<std::uint32_t> m_queueFamilyIndices;
	VkResourceView m_modelBuffers;
	std::uint32_t m_bufferCount;
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
};
#endif
