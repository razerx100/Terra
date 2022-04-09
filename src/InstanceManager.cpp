#include <InstanceManager.hpp>

void DebugLayerInst::Init(VkInstance instanceRef) {
	Set(
		CreateDebugLayerInstance(instanceRef)
	);
}

void GfxPoolInst::Init(
	VkDevice device, size_t queueIndex, size_t bufferCount
) {
	Set(
		CreateCommandPoolInstance(device, queueIndex, bufferCount)
	);
}

void DeviceInst::Init() {
	Set(
		CreateDeviceManagerInstance()
	);
}

void VkInstInst::Init(const char* appName) {
	Set(
		CreateInstanceManagerInstance(appName)
	);
}

void GfxQueInst::Init(VkDevice device, VkQueue queue, size_t bufferCount) {
	Set(
		CreateGraphicsQueueManagerInstance(device, queue, bufferCount)
	);
}

void SwapChainInst::Init(
	VkDevice device, const SwapChainInfo& swapCapabilities, VkSurfaceKHR surface,
	std::uint32_t width, std::uint32_t height, size_t bufferCount,
	VkQueue presentQueue, size_t queueFamily
) {
	Set(
		CreateSwapchainManagerInstance(
			device, swapCapabilities, surface,
			width, height, bufferCount,
			presentQueue, queueFamily
		)
	);
}

void DisplayInst::Init() {
	Set(
		CreateDisplayManagerInstance()
	);
}

#ifdef TERRA_WIN32
void SurfaceInst::InitWin32(
	VkInstance instance, void* windowHandle, void* moduleHandle
) {
	Set(
		CreateWin32SurfaceManagerInstance(instance, windowHandle, moduleHandle)
	);
}
#endif

void CpyPoolInst::Init(
	VkDevice device, size_t queueIndex
) {
	Set(
		CreateCommandPoolInstance(device, queueIndex, 1u)
	);
}

void CpyQueInst::Init(VkDevice device, VkQueue queue) {
	Set(
		CreateCopyQueueManagerInstance(device, queue)
	);
}

void VertexBufferInst::Init(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices
) {
	Set(
		CreateResourceBufferInstance(
			logDevice, phyDevice, queueFamilyIndices, BufferType::Vertex
		)
	);
}

void IndexBufferInst::Init(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices
) {
	Set(
		CreateResourceBufferInstance(
			logDevice, phyDevice, queueFamilyIndices, BufferType::Index
		)
	);
}

void ViewPAndScsrInst::Init(std::uint32_t width, std::uint32_t height) {
	Set(
		CreateViewportAndScissorInstance(width, height)
	);
}

void RndrPassInst::Init(VkDevice device, VkFormat swapchainFormat) {
	Set(
		CreateRenderPassManagerInstance(device, swapchainFormat)
	);
}

void ModelContainerInst::Init(const char* shaderPath) {
	Set(
		CreateModelContainerInstance(shaderPath)
	);
}

void UniformBufferInst::Init(
	VkDevice logDevice, VkPhysicalDevice phyDevice,
	const std::vector<std::uint32_t>& queueFamilyIndices
) {
	Set(
		CreateResourceBufferInstance(
			logDevice, phyDevice, queueFamilyIndices, BufferType::UniformAndStorage
		)
	);
}

void DescSetMan::Init(VkDevice device) {
	Set(
		CreateDescriptorSetManagerInstance(device)
	);
}
