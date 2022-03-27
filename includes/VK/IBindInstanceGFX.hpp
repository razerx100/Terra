#ifndef __I_BIND_INSTANCE_GFX_HPP__
#define __I_BIND_INSTANCE_GFX_HPP__
#include <vulkan/vulkan.hpp>
#include <IModel.hpp>
#include <IPipelineObject.hpp>
#include <IPipelineLayout.hpp>
#include <VertexLayout.hpp>
#include <memory>

class IBindInstanceGFX {
public:
	virtual ~IBindInstanceGFX() = default;

	virtual VertexLayout GetVertexLayout() const noexcept = 0;

	virtual void AddPSO(std::unique_ptr<IPipelineObject> pso) noexcept = 0;
	virtual void AddPipelineLayout(std::shared_ptr<IPipelineLayout> layout) noexcept = 0;
	virtual void AddModel(
		VkDevice device, const IModel* const modelRef
	) noexcept = 0;

	virtual void BindCommands(VkCommandBuffer commandBuffer) noexcept = 0;
};
#endif
