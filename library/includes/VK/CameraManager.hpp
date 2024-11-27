#ifndef CAMERA_MANAGER_HPP_
#define CAMERA_MANAGER_HPP_
#include <cstdint>
#include <memory>
#include <vector>
#include <VkResources.hpp>
#include <VkDescriptorBuffer.hpp>

#include <Camera.hpp>
#include <DirectXMath.h>

class CameraManager
{
public:
	CameraManager(VkDevice device, MemoryManager* memoryManager);

	[[nodiscard]]
	std::uint32_t AddCamera(std::shared_ptr<Camera> camera) noexcept;

	void SetCamera(std::uint32_t index) noexcept { m_activeCameraIndex = index; }

	void RemoveCamera(std::uint32_t index) noexcept;

	void CreateBuffer(
		const std::vector<std::uint32_t>& queueIndices, std::uint32_t frameCount
	);

	void Update(VkDeviceSize index) const noexcept;

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
	size_t                               m_activeCameraIndex;
	VkDeviceSize                         m_cameraBufferInstanceSize;
	Buffer                               m_cameraBuffer;
	std::vector<std::shared_ptr<Camera>> m_cameras;

public:
	CameraManager(const CameraManager&) = delete;
	CameraManager& operator=(const CameraManager&) = delete;

	CameraManager(CameraManager&& other) noexcept
		: m_activeCameraIndex{ other.m_activeCameraIndex },
		m_cameraBufferInstanceSize{ other.m_cameraBufferInstanceSize },
		m_cameraBuffer{ std::move(other.m_cameraBuffer) },
		m_cameras{ std::move(other.m_cameras) }
	{}
	CameraManager& operator=(CameraManager&& other) noexcept
	{
		m_activeCameraIndex        = other.m_activeCameraIndex;
		m_cameraBufferInstanceSize = other.m_cameraBufferInstanceSize;
		m_cameraBuffer             = std::move(other.m_cameraBuffer);
		m_cameras                  = std::move(other.m_cameras);

		return *this;
	}
};
#endif
