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
		std::optional<bool> modelDataNoBB;
	};

public:
	BufferManager(Args& arguments);

	void CreateBuffers(VkDevice device) noexcept;
	void AddOpaqueModels(std::vector<std::shared_ptr<IModel>>&& models) noexcept;
	void BindResourceToMemory(VkDevice device) const noexcept;

	template<bool modelWithNoBB>
	void Update(VkDeviceSize bufferIndex) const noexcept {
		std::uint8_t* cpuMemoryStart = GetCPUWriteStartMemory();
		const DirectX::XMMATRIX viewMatrix = GetViewMatrix();

		UpdateCameraData(bufferIndex, cpuMemoryStart);
		UpdatePerModelData<modelWithNoBB>(bufferIndex, cpuMemoryStart, viewMatrix);
		UpdateLightData(bufferIndex, cpuMemoryStart, viewMatrix);
		UpdateFragmentData(bufferIndex, cpuMemoryStart);
	}

private:
	struct ModelBuffer {
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMMATRIX viewNormalMatrix;
		DirectX::XMFLOAT3 modelOffset;
		float padding1;
		DirectX::XMFLOAT3 positiveBounds;
		float padding2;
		DirectX::XMFLOAT3 negativeBounds;
		float padding3;
		// GLSL's vec3 is actually vec4.
	};

	struct ModelBufferNoBB {
		DirectX::XMMATRIX modelMatrix;
		DirectX::XMMATRIX viewNormalMatrix;
		DirectX::XMFLOAT3 modelOffset;
		float padding1;
		// GLSL's vec3 is actually vec4.
	};

	struct MaterialBuffer {
		DirectX::XMFLOAT4 ambient;
		DirectX::XMFLOAT4 diffuse;
		DirectX::XMFLOAT4 specular;
		UVInfo diffuseTexUVInfo;
		UVInfo specularTexUVInfo;
		std::uint32_t diffuseTexIndex;
		std::uint32_t specularTexIndex;
		float shininess;
		float padding;
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

private:
	void UpdateCameraData(
		VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart
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

	[[nodiscard]]
	DirectX::XMMATRIX GetViewMatrix() const noexcept;
	[[nodiscard]]
	std::uint8_t* GetCPUWriteStartMemory() const noexcept;

	void CheckLightSourceAndAddOpaque(std::shared_ptr<IModel>&& model) noexcept;

	template<bool modelWithNoBB>
	void UpdatePerModelData(
		VkDeviceSize bufferIndex, std::uint8_t* cpuMemoryStart,
		const DirectX::XMMATRIX& viewMatrix
	) const noexcept {
		size_t modelOffset = 0u;
		size_t materialOffset = 0u;

		std::uint8_t* modelBuffersOffset =
			cpuMemoryStart + m_modelBuffers.GetMemoryOffset(bufferIndex);
		std::uint8_t* materialBuffersOffset =
			cpuMemoryStart + m_materialBuffers.GetMemoryOffset(bufferIndex);

		for (auto& model : m_opaqueModels) {
			const auto& boundingBox = model->GetBoundingBox();
			const DirectX::XMMATRIX modelMatrix = model->GetModelMatrix();

			if constexpr (modelWithNoBB)
				CopyStruct(
					ModelBufferNoBB{
					.modelMatrix = modelMatrix,
					.viewNormalMatrix = DirectX::XMMatrixTranspose(
						DirectX::XMMatrixInverse(nullptr, modelMatrix * viewMatrix)
					),
					.modelOffset = model->GetModelOffset()
					},
					modelBuffersOffset, modelOffset
				);
			else
				CopyStruct(
					ModelBuffer{
					.modelMatrix = modelMatrix,
					.viewNormalMatrix = DirectX::XMMatrixTranspose(
						DirectX::XMMatrixInverse(nullptr, modelMatrix * viewMatrix)
					),
					.modelOffset = model->GetModelOffset(),
					.positiveBounds = boundingBox.positiveAxes,
					.negativeBounds = boundingBox.negativeAxes
					},
					modelBuffersOffset, modelOffset
				);

			const auto& modelMaterial = model->GetMaterial();

			MaterialBuffer material{
				.ambient = modelMaterial.ambient,
				.diffuse = modelMaterial.diffuse,
				.specular = modelMaterial.specular,
				.diffuseTexUVInfo = model->GetDiffuseTexUVInfo(),
				.specularTexUVInfo = model->GetSpecularTexUVInfo(),
				.diffuseTexIndex = model->GetDiffuseTexIndex(),
				.specularTexIndex = model->GetSpecularTexIndex(),
				.shininess = modelMaterial.shininess
			};
			CopyStruct(material, materialBuffersOffset, materialOffset);
		}
	}

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
	bool m_modelDataNoBB;
};
#endif
