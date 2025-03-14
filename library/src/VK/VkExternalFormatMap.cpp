#include <array>
#include <VkExternalFormatMap.hpp>

static constexpr std::array s_externalFormatMap
{
	VK_FORMAT_UNDEFINED,
	VK_FORMAT_R8G8B8A8_UNORM,
	VK_FORMAT_R8G8B8A8_SRGB,
	VK_FORMAT_B8G8R8A8_UNORM,
	VK_FORMAT_B8G8R8A8_SRGB,
	VK_FORMAT_D32_SFLOAT,
	VK_FORMAT_D24_UNORM_S8_UINT,
	VK_FORMAT_S8_UINT
};

VkFormat GetVkFormat(ExternalFormat format) noexcept
{
	return s_externalFormatMap[static_cast<size_t>(format)];
}

ExternalFormat GetExternalFormat(VkFormat format) noexcept
{
	size_t formatIndex = 0u;

	for (size_t index = 0u; index < std::size(s_externalFormatMap); ++index)
		if (s_externalFormatMap[index] == format)
		{
			formatIndex = index;

			break;
		}

	return static_cast<ExternalFormat>(formatIndex);
}
