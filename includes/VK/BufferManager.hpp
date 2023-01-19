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

class BufferManager {
public:
	struct Args {
		std::optional<VkDevice> device;
		std::optional<std::uint32_t> bufferCount;
		std::optional<QueueIndicesCG> queueIndices;
	};

public:
	BufferManager(Args& arguments);

	void Update(VkDeviceSize bufferIndex) const noexcept;
	void CreateBuffers(VkDevice device) noexcept;
	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void BindResourceToMemory(VkDevice device) const noexcept;

private:
	void UpdateCameraData(
		VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart
	) const noexcept;
	void UpdatePerModelData(
		VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart,
		const DirectX::XMMATRIX& viewMatrix
	) const noexcept;
	void UpdateLightData(
		VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart,
		const DirectX::XMMATRIX& viewMatrix
	) const noexcept;
	void UpdateFragmentData(
		VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart
	) const noexcept;

	void CreateBufferComputeAndGraphics(
		VkDevice device, VkResourceView& buffer, VkDeviceSize bufferSize,
		VkBufferUsageFlagBits bufferType, const DescriptorInfo& descInfo,
		const std::vector<std::uint32_t>& resolvedQueueIndices
	) const noexcept;
	void CreateBufferGraphics(
		VkDevice device, VkResourceView& buffer, VkDeviceSize bufferSize,
		VkBufferUsageFlagBits bufferType, const DescriptorInfo& descInfo
	) const noexcept;

	template<typename T>
	void CopyStruct(
		const T& data, std::uint8_t* offsetInMemory, size_t& currentOffset
	) const noexcept {
		static constexpr size_t stride = sizeof(T);

		memcpy(offsetInMemory + currentOffset, &data, stride);
		currentOffset += stride;
	}

private:
	VkResourceView m_cameraBuffer;
	VkResourceView m_modelBuffers;
	VkResourceView m_materialBuffers;
	VkResourceView m_lightBuffers;
	VkResourceView m_fragmentDataBuffer;
	std::uint32_t m_bufferCount;
	std::vector<std::shared_ptr<IModel>> m_opaqueModels;
	QueueIndicesCG m_queueIndices;
	std::vector<size_t> m_lightModelIndices;
};

struct ModelBuffer {
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
	DirectX::XMMATRIX viewNormalMatrix;
	// GLSL's vec3 is actually vec4.
};

struct MaterialBuffer{
	DirectX::XMFLOAT4 ambient;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
	float shininess;
	float padding[3];
};

struct LightBuffer {
	DirectX::XMFLOAT3 position;
	float padding;
	DirectX::XMFLOAT4 ambient;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular;
};

struct FragmentData {
	std::uint32_t lightCount;
};
#endif
