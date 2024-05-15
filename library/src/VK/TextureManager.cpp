#include <TextureManager.hpp>

// Texture storage
void TextureStorage::SetBindingIndex(size_t textureIndex, std::uint32_t bindingIndex) noexcept
{
	if (std::size(m_textureBindingIndices) <= textureIndex)
	{
		m_textureBindingIndices.resize(textureIndex + 1u);
	}

	m_textureBindingIndices.at(textureIndex) = bindingIndex;
}

