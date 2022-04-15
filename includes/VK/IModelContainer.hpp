#ifndef __I_MODEL_CONTAINER_HPP__
#define __I_MODEL_CONTAINER_HPP__
#include <vulkan/vulkan.hpp>
#include <IModel.hpp>
#include <atomic>

class IModelContainer {
public:
	virtual ~IModelContainer() = default;

	virtual void AddModel(
		VkDevice device, const IModel* const modelRef
	) = 0;

	virtual void InitPipelines(VkDevice device) = 0;
	virtual void CreateBuffers(VkDevice device) = 0;
	virtual void CopyData(std::atomic_size_t& workCount) = 0;
	virtual void RecordUploadBuffers(VkDevice device, VkCommandBuffer copyBuffer) = 0;
	virtual void ReleaseUploadBuffers() = 0;

	virtual void BindCommands(VkCommandBuffer commandBuffer) noexcept = 0;
};

IModelContainer* CreateModelContainerInstance(
	const char* shaderPath
);

#endif