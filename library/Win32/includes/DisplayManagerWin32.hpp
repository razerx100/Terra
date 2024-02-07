#ifndef DISPLAY_MANAGER_WIN32_HPP_
#define DISPLAY_MANAGER_WIN32_HPP_
#include <IDisplayManager.hpp>
#include <CleanWin.hpp>
#include <wrl/client.h>
#include <dxgi1_6.h>

using Microsoft::WRL::ComPtr;

class DisplayManagerWin32 final : public IDisplayManager {
public:
	DisplayManagerWin32();

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
	ComPtr<IDXGIFactory1> m_pFactory;

	inline static const std::vector<InstanceExtension> s_requiredExtensions
	{
		InstanceExtension::VkKhrExternalMemoryCapabilities
	};

public:
	[[nodiscard]]
	const std::vector<InstanceExtension>& GetRequiredExtensions() const noexcept override
	{ return s_requiredExtensions; }
};

#endif
