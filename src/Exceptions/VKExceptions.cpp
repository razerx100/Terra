#include <VKExceptions.hpp>
#include <sstream>
#include <vector>

VKException::VKException(int line, const char* file, VkResult errorCode) noexcept
	: Exception(line, file), m_errorCode(errorCode) {
	GenerateWhatBuffer();
}

void VKException::GenerateWhatBuffer() noexcept {
	std::ostringstream oss;
	oss << GetType() << "\n"
		<< "[Error code] " << m_errorCode << "\n\n"
		<< "[Error String] " << GetErrorString() << "\n";
	oss << GetOriginString();
	m_whatBuffer = oss.str();
}

const char* VKException::what() const noexcept {
	return m_whatBuffer.c_str();
}

const char* VKException::GetType() const noexcept {
	return "Graphics Exception";
}

std::string VKException::GetErrorString() const noexcept {
#define STR(str) case str : return #str

	switch (m_errorCode) {
		STR(VK_ERROR_OUT_OF_HOST_MEMORY);
		STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
		STR(VK_ERROR_INITIALIZATION_FAILED);
		STR(VK_ERROR_DEVICE_LOST);
		STR(VK_ERROR_MEMORY_MAP_FAILED);
		STR(VK_ERROR_LAYER_NOT_PRESENT);
		STR(VK_ERROR_EXTENSION_NOT_PRESENT);
		STR(VK_ERROR_FEATURE_NOT_PRESENT);
		STR(VK_ERROR_INCOMPATIBLE_DRIVER);
		STR(VK_ERROR_TOO_MANY_OBJECTS);
		STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
		STR(VK_ERROR_FRAGMENTED_POOL);
		STR(VK_ERROR_UNKNOWN);
		STR(VK_ERROR_OUT_OF_POOL_MEMORY);
		STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
		STR(VK_ERROR_FRAGMENTATION);
		STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS);
		STR(VK_ERROR_SURFACE_LOST_KHR);
		STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(VK_ERROR_OUT_OF_DATE_KHR);
		STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(VK_ERROR_VALIDATION_FAILED_EXT);
		STR(VK_ERROR_INVALID_SHADER_NV);
		STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT);
		STR(VK_ERROR_NOT_PERMITTED_EXT);
		STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT);
	default:
		return "Invalid Error";
	}
}
