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
	void CreateBuffers(VkDevice device) noexcept;
	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void BindResourceToMemory(VkDevice device) const noexcept;

private:
	void UpdateModelData(VkDeviceSize index) const noexcept;

private:
	VkResourceView m_cameraBuffer;
	VkResourceView m_modelBuffers;
	std::uint32_t m_bufferCount;
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
	std::vector<std::uint32_t> m_computeAndGraphicsQueueIndices;
};
#endif
