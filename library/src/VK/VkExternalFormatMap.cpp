#include <array>
#include <VkExternalFormatMap.hpp>

static constexpr std::array sExternalFormatMap
{
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_D32_SFLOAT
};

VkFormat GetVkFormat(ExternalFormat format) noexcept
{
	return sExternalFormatMap[static_cast<size_t>(format)];
}
