#include <DisplayManagerWin32.hpp>
#include <cassert>

using Microsoft::WRL::ComPtr;

void DisplayInstanceExtensionWin32::SetInstanceExtensions(
	VkInstanceExtensionManager& extensionManager
) noexcept {
	extensionManager.AddExtension(InstanceExtension::VkKhrExternalMemoryCapabilities);
}

// Display Manager
DisplayManagerWin32::DisplayManagerWin32()
{
	CreateDXGIFactory2(0u, IID_PPV_ARGS(&m_pFactory));
}

VkExtent2D DisplayManagerWin32::GetDisplayResolution(
	VkPhysicalDevice gpu, std::uint32_t displayIndex
) const {
	DXGI_ADAPTER_DESC gpuDesc{};
	LUID gpuLUid{};
	GetLUIDFromVKDevice(gpu, gpuLUid);

	ComPtr<IDXGIAdapter1> d3dGpu = GetAdapter(gpuLUid);

	assert(d3dGpu && "GPU IDs don't match.");

	d3dGpu->GetDesc(&gpuDesc);

	ComPtr<IDXGIOutput> pDisplayOutput{};
	[[maybe_unused]] HRESULT displayCheck = d3dGpu->EnumOutputs(displayIndex, &pDisplayOutput);
	assert(SUCCEEDED(displayCheck) && "Invalid display index.");

	DXGI_OUTPUT_DESC displayData{};
	pDisplayOutput->GetDesc(&displayData);

	return VkExtent2D
	{
		.width  = static_cast<std::uint32_t>(displayData.DesktopCoordinates.right),
		.height = static_cast<std::uint32_t>(displayData.DesktopCoordinates.bottom)
	};
}

ComPtr<IDXGIAdapter1> DisplayManagerWin32::GetAdapter(const LUID& adapterLUid) const noexcept
{
	ComPtr<IDXGIAdapter1> pAdapter{};
	DXGI_ADAPTER_DESC gpuDesc{};

	for (UINT index = 0u; m_pFactory->EnumAdapters1(index, &pAdapter) != DXGI_ERROR_NOT_FOUND;)
	{
		pAdapter->GetDesc(&gpuDesc);
		if (AreLUIDsSame(gpuDesc.AdapterLuid, adapterLUid))
			return pAdapter;
	}

	return nullptr;
}

bool DisplayManagerWin32::AreLUIDsSame(const LUID& lUid1, const LUID& lUid2) const noexcept
{
	if (lUid1.LowPart == lUid2.LowPart && lUid1.HighPart == lUid2.HighPart)
		return true;

	return false;
}

void DisplayManagerWin32::GetLUIDFromVKDevice(VkPhysicalDevice gpu, LUID& lUid) const
{
	VkPhysicalDeviceIDProperties gpuIDS
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES
	};

	VkPhysicalDeviceProperties2 gpuProperties
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
		.pNext = &gpuIDS
	};

	vkGetPhysicalDeviceProperties2(gpu, &gpuProperties);

	assert(gpuIDS.deviceLUIDValid == VK_TRUE && "Couldn't retrieve LUID.");

	lUid = *reinterpret_cast<LUID*>(gpuIDS.deviceLUID);
}
