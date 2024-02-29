#include <SurfaceManager.hpp>

void SurfaceManager::SelfDestruct() noexcept
{
	if (m_instance)
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

