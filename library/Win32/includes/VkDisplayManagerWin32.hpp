#ifndef VK_DISPLAY_MANAGER_WIN32_HPP_
#define VK_DISPLAY_MANAGER_WIN32_HPP_
#include <vulkan/vulkan.hpp>
#include <array>
#include <VkExtensionManager.hpp>
#include <CleanWin.hpp>
#include <wrl/client.h>
#include <dxgi1_6.h>

namespace DisplayInstanceExtensionWin32
{
	void SetInstanceExtensions(VkInstanceExtensionManager& extensionManager) noexcept;
};

class DisplayManagerWin32
{
public:
	DisplayManagerWin32();

	[[nodiscard]]
	VkExtent2D GetDisplayResolution(VkPhysicalDevice gpu, std::uint32_t displayIndex) const;

private:
	[[nodiscard]]
	bool AreLUIDsSame(const LUID& lUid1, const LUID& lUid2) const noexcept;
	[[nodiscard]]
	Microsoft::WRL::ComPtr<IDXGIAdapter1> GetAdapter(const LUID& adapterLUid) const noexcept;

	void GetLUIDFromVKDevice(VkPhysicalDevice gpu, LUID& lUid) const;

private:
	Microsoft::WRL::ComPtr<IDXGIFactory1> m_pFactory;
};
#endif
