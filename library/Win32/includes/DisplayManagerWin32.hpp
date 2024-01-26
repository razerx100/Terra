#ifndef DISPLAY_MANAGER_WIN32_HPP_
#define DISPLAY_MANAGER_WIN32_HPP_
#include <IDisplayManager.hpp>
#include <wrl/client.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;

class DisplayManagerWin32 final : public IDisplayManager {
public:
	DisplayManagerWin32();

	[[nodiscard]]
	const std::vector<const char*>& GetRequiredExtensions() const noexcept override;
	[[nodiscard]]
	Resolution GetDisplayResolution(
		VkPhysicalDevice gpu, std::uint32_t displayIndex
	) const override;

private:
	[[nodiscard]]
	bool AreLUIDsSame(const LUID& lUid1, const LUID& lUid2) const noexcept;
	[[nodiscard]]
	ComPtr<IDXGIAdapter1> GetAdapter(const LUID& adapterLUid) const noexcept;

	void GetLUIDFromVKDevice(VkPhysicalDevice gpu, LUID& lUid) const;

private:
	const std::vector<const char*> m_requiredExtensions = {
		"VK_KHR_external_memory_capabilities"
	};

	ComPtr<IDXGIFactory1> m_pFactory;
};

#endif
