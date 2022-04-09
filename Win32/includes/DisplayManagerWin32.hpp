#ifndef __DISPLAY_MANAGER_WIN32_HPP__
#define __DISPLAY_MANAGER_WIN32_HPP__
#include <IDisplayManager.hpp>
#include <wrl/client.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;

class DisplayManagerWin32 : public IDisplayManager {
public:
	DisplayManagerWin32();

	void InitDisplayManager(VkPhysicalDevice gpu) override;

	[[nodiscard]]
	const std::vector<const char*>& GetRequiredExtensions() const noexcept override;
	void GetDisplayResolution(VkPhysicalDevice gpu, Ceres::Rect& displayRect) override;

private:
	[[nodiscard]]
	bool AreLUIDsSame(const LUID& lUid1, const LUID& lUid2) const noexcept;

	void MatchAdapter(const LUID& adapterLUid);
	void GetLUIDFromVKDevice(VkPhysicalDevice gpu, LUID& lUid) const;

private:
	const std::vector<const char*> m_requiredExtensions = {
		"VK_KHR_get_physical_device_properties2",
		"VK_KHR_external_memory_capabilities"
	};

	ComPtr<IDXGIFactory1> m_pFactory;
	ComPtr<IDXGIAdapter1> m_pD3DGPU;
};

#endif
