#ifndef BUFFER_MANAGER_HPP_
#define BUFFER_MANAGER_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <cstdint>
#include <VkResourceViews.hpp>
#include <DescriptorSetManager.hpp>
#include <VkHelperFunctions.hpp>
#include <optional>

#include <IModel.hpp>

struct ModelConstantBuffer {
	UVInfo uvInfo;
	DirectX::XMMATRIX modelMatrix;
	std::uint32_t textureIndex;
	float padding0[3];
	DirectX::XMFLOAT3 modelOffset;
	float padding1;
	DirectX::XMFLOAT3 positiveBounds;
	float padding2;
	DirectX::XMFLOAT3 negativeBounds;
	float padding3;
	// GLSL's vec3 is actually vec4.
};

class BufferManager {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<std::uint32_t> bufferCount;
		std::optional<QueueIndicesCG> queueIndices;
	};

public:
	BufferManager(Args& arguments);

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
	QueueIndicesCG m_queueIndices;
};
#endif
