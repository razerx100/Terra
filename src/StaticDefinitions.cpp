#include <GraphicsEngineVK.hpp>
#include <InstanceManager.hpp>
#include <DebugLayerManager.hpp>

static GraphicsEngine* s_pGraphicsEngine = nullptr;

static InstanceManager* s_pInstanceManager = nullptr;

static DebugLayerManager* s_pDebugLayerManager = nullptr;

// Graphics Engine
GraphicsEngine* GetGraphicsEngineInstance() noexcept {
	return s_pGraphicsEngine;
}

void InitGraphicsEngineInstance(
	const char* appName,
	void* windowHandle,
	void* moduleHandle,
	std::uint32_t width, std::uint32_t height,
	std::uint8_t bufferCount
) {
	if (!s_pGraphicsEngine)
		s_pGraphicsEngine = new GraphicsEngineVK(
			appName,
			windowHandle, moduleHandle, width, height, bufferCount
		);
}

void CleanUpGraphicsEngineInstance() noexcept {
	if (s_pGraphicsEngine)
		delete s_pGraphicsEngine;
}

// Instance Manager
InstanceManager* GetVKInstance() noexcept {
	return s_pInstanceManager;
}

void InitVKInstance(const char* appName) {
	if (!s_pInstanceManager)
		s_pInstanceManager = new InstanceManager(appName);
}

void CleanUpVKInstance() noexcept {
	if (s_pInstanceManager)
		delete s_pInstanceManager;
}

// Debug Layer
void InitDebugLayer(VkInstance instanceRef) {
	if (!s_pDebugLayerManager)
		s_pDebugLayerManager = new DebugLayerManager(instanceRef);
}

void CleanUpDebugLayer() noexcept {
	if (s_pDebugLayerManager)
		delete s_pDebugLayerManager;
}
