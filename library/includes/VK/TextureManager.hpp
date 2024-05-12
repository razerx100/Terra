#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_
#include <VkResources.hpp>
#include <StagingBufferManager.hpp>
#include <VkDescriptorBuffer.hpp>

class TextureManager
{
public:

private:
	std::vector<VkTextureView> m_textures;
};
#endif
