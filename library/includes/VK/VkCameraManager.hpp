#ifndef VK_CAMERA_MANAGER_HPP_
#define VK_CAMERA_MANAGER_HPP_
#include <cstdint>
#include <memory>
#include <vector>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>

#include <Camera.hpp>
#include <DirectXMath.h>

namespace Terra
{
class CameraManager
{
public:
	CameraManager(VkDevice device, MemoryManager* memoryManager);

	void CreateBuffer(
		const std::vector<std::uint32_t>& queueIndices, std::uint32_t frameCount
	);

	void Update(VkDeviceSize index, const Camera& cameraData) const noexcept;

	void SetDescriptorBufferLayoutGraphics(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
		size_t setLayoutIndex, VkShaderStageFlags shaderStage
	) const noexcept;
	void SetDescriptorBufferLayoutCompute(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
		size_t setLayoutIndex
	) const noexcept;

	void SetDescriptorBufferGraphics(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
		size_t setLayoutIndex
	) const;
	void SetDescriptorBufferCompute(
		std::vector<VkDescriptorBuffer>& descriptorBuffers, std::uint32_t cameraBindingSlot,
		size_t setLayoutIndex
	) const;

private:
	// Not actually using this. Just using it to get the struct size.
	struct CameraBufferData
	{
		DirectX::XMMATRIX view;
		DirectX::XMMATRIX projection;
		Frustum           viewFrustum;
		DirectX::XMFLOAT4 viewPosition;
	};

private:
	size_t       m_activeCameraIndex;
	VkDeviceSize m_cameraBufferInstanceSize;
	Buffer       m_cameraBuffer;

public:
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	CameraManager(CameraManager&& other) noexcept
		: m_activeCameraIndex{ other.m_activeCameraIndex },
		m_cameraBufferInstanceSize{ other.m_cameraBufferInstanceSize },
		m_cameraBuffer{ std::move(other.m_cameraBuffer) }
	{}
	CameraManager& operator=(CameraManager&& other) noexcept
	{
		m_activeCameraIndex        = other.m_activeCameraIndex;
		m_cameraBufferInstanceSize = other.m_cameraBufferInstanceSize;
		m_cameraBuffer             = std::move(other.m_cameraBuffer);

		return *this;
	}
};
}
#endif
