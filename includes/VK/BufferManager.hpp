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
	float padding0[3];
	DirectX::XMFLOAT3 modelOffset;
	DirectX::XMFLOAT4 boundingBox[2u]; // GLSL's vec3 is actually vec4.
	// So, Float4 should be used when contiguous vec3s are required. Float3 is fine
	// for single vec3
};

class BufferManager {
public:
	BufferManager(
		VkDevice device, std::uint32_t bufferCount,
		std::vector<std::uint32_t> computeAndGraphicsQueueIndices
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
	void AcquireOwnerShips(
		VkCommandBuffer cmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;
	void ReleaseOwnerships(
		VkCommandBuffer copyCmdBuffer, std::uint32_t srcQueueIndex, std::uint32_t dstQueueIndex
	) noexcept;
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
	VkResourceView m_modelBuffers;
	std::uint32_t m_bufferCount;
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
	std::vector<std::uint32_t> m_computeAndGraphicsQueueIndices;
};
#endif
