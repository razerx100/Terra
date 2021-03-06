#include <DisplayManagerWin32.hpp>
#include <VKThrowMacros.hpp>

DisplayManagerWin32::DisplayManagerWin32() {
	CreateDXGIFactory2(0u, __uuidof(IDXGIFactory1), &m_pFactory);
}

const std::vector<const char*>& DisplayManagerWin32::GetRequiredExtensions() const noexcept {
	return m_requiredExtensions;
}

IDisplayManager::Resolution DisplayManagerWin32::GetDisplayResolution(
	VkPhysicalDevice gpu, std::uint32_t displayIndex
) const {
	DXGI_ADAPTER_DESC gpuDesc = {};
	LUID gpuLUid;
	GetLUIDFromVKDevice(gpu, gpuLUid);

	ComPtr<IDXGIAdapter1> d3dGpu = GetAdapter(gpuLUid);

	if (!d3dGpu)
		VK_GENERIC_THROW("GPU ID doesn't match.");

	d3dGpu->GetDesc(&gpuDesc);

	ComPtr<IDXGIOutput> pDisplayOutput;
	if (FAILED(d3dGpu->EnumOutputs(displayIndex, &pDisplayOutput)))
		VK_GENERIC_THROW("Searched GPU couldn't be found.");

	DXGI_OUTPUT_DESC displayData = {};
	pDisplayOutput->GetDesc(&displayData);

	return {
		static_cast<std::uint64_t>(displayData.DesktopCoordinates.right),
		static_cast<std::uint64_t>(displayData.DesktopCoordinates.bottom)
	};
}

ComPtr<IDXGIAdapter1> DisplayManagerWin32::GetAdapter(const LUID& adapterLUid) const noexcept {
	ComPtr<IDXGIAdapter1> pAdapter;
	DXGI_ADAPTER_DESC gpuDesc = {};
	for (UINT index = 0u;
		m_pFactory->EnumAdapters1(index, &pAdapter) != DXGI_ERROR_NOT_FOUND;) {

		pAdapter->GetDesc(&gpuDesc);
		if (AreLUIDsSame(gpuDesc.AdapterLuid, adapterLUid))
			return pAdapter;
	}

	return nullptr;
}

bool DisplayManagerWin32::AreLUIDsSame(const LUID& lUid1, const LUID& lUid2) const noexcept {
	if (lUid1.LowPart == lUid2.LowPart && lUid1.HighPart == lUid2.HighPart)
		return true;

	return false;
}

void DisplayManagerWin32::GetLUIDFromVKDevice(VkPhysicalDevice gpu, LUID& lUid) const {
	VkPhysicalDeviceProperties2 gpuProperties = {};
	gpuProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	VkPhysicalDeviceIDProperties gpuIDS = {};
	gpuIDS.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;
	gpuProperties.pNext = &gpuIDS;

	vkGetPhysicalDeviceProperties2(gpu, &gpuProperties);

	lUid = {};
	if (gpuIDS.deviceLUIDValid == VK_TRUE)
		lUid = *reinterpret_cast<LUID*>(gpuIDS.deviceLUID);
	else
		VK_GENERIC_THROW("Couldn't retrieve LUID.");
}
