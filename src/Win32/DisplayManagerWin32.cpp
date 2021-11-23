#include <Win32/DisplayManagerWin32.hpp>

DisplayManagerWin32::DisplayManagerWin32(VkPhysicalDevice gpu) {
	CreateDXGIFactory2(0u, __uuidof(IDXGIFactory1), &m_pFactory);
	LUID gpuLUid;
	GetLUIDFromVKDevice(gpu, gpuLUid);
	MatchAdapter(gpuLUid);
}

const std::vector<const char*>& DisplayManagerWin32::GetRequiredExtensions() const noexcept {
	return m_requiredExtensions;
}

void DisplayManagerWin32::GetDisplayResolution(VkPhysicalDevice gpu, SRect& displayRect) {
	DXGI_ADAPTER_DESC gpuDesc = {};
	LUID gpuLUid;
	GetLUIDFromVKDevice(gpu, gpuLUid);
	m_pD3DGPU->GetDesc(&gpuDesc);

	if (!AreLUIDsSame(gpuDesc.AdapterLuid, gpuLUid))
		MatchAdapter(gpuLUid);

	ComPtr<IDXGIOutput> pDisplayOutput;
	m_pD3DGPU->EnumOutputs(0u, &pDisplayOutput);

	DXGI_OUTPUT_DESC displayData = {};
	pDisplayOutput->GetDesc(&displayData);

	displayRect = *reinterpret_cast<SRect*>(&displayData.DesktopCoordinates);
}

void DisplayManagerWin32::MatchAdapter(const LUID& adapterLUid) {
	ComPtr<IDXGIAdapter1> pAdapter;
	DXGI_ADAPTER_DESC gpuDesc = {};
	for (std::uint32_t index = 0u;
		m_pFactory->EnumAdapters1(index, &pAdapter) != DXGI_ERROR_NOT_FOUND;) {

		pAdapter->GetDesc(&gpuDesc);
		if (AreLUIDsSame(gpuDesc.AdapterLuid, adapterLUid)) {
			m_pD3DGPU.Swap(pAdapter);
			break;
		}
	}
}

bool DisplayManagerWin32::AreLUIDsSame(const LUID& lUid1, const LUID& lUid2) const noexcept {
	if (lUid1.LowPart == lUid2.LowPart && lUid1.HighPart == lUid2.HighPart)
		return true;

	return false;
}

void DisplayManagerWin32::GetLUIDFromVKDevice(VkPhysicalDevice gpu, LUID& lUid) const noexcept {
	VkPhysicalDeviceProperties2 gpuProperties = {};
	VkPhysicalDeviceIDProperties gpuIDS = {};
	gpuProperties.pNext = &gpuIDS;

	vkGetPhysicalDeviceProperties2(gpu, &gpuProperties);

	lUid = {};
	if (gpuIDS.deviceLUIDValid == VK_TRUE)
		lUid = *reinterpret_cast<LUID*>(gpuIDS.deviceLUID);
}
