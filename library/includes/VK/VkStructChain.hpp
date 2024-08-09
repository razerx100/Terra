#ifndef VK_STRUCT_CHAIN_HPP_
#define VK_STRUCT_CHAIN_HPP_
#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>

template<class T>
class VkStructChain
{
public:
	VkStructChain(VkStructureType baseStructType) : m_baseStruct{ .sType = baseStructType } {}

	template<class G>
	void AddToChain(std::shared_ptr<G>&& chainStruct) noexcept
	{
		if (m_lastInChain)
		{
			m_chainedStructs.emplace_back(m_lastInChain);

			auto structStart   = reinterpret_cast<std::uint8_t*>(m_lastInChain.get());
			// In a vulkan structure the first member is always sType and the second one is pNext.
			// But because of the padding between struct members. Just adding the size of VkStructureType
			// wouldn't work. And I shouldn't hardcode the offset by including the padding since
			// it might be different on different architectures.
			struct PNextStruct
			{
				VkStructureType sType;
				void*           pNext;
			};

			auto* pNextStruct  = reinterpret_cast<PNextStruct*>(structStart);
			pNextStruct->pNext = chainStruct.get();
			m_lastInChain      = std::move(chainStruct);
		}
		else
		{
			m_baseStruct.pNext = chainStruct.get();
			m_lastInChain      = std::move(chainStruct);
		}
	}

	[[nodiscard]]
	const T* GetPtr() const noexcept { return &m_baseStruct; }
	[[nodiscard]]
	T& Get() noexcept { return m_baseStruct; }
	[[nodiscard]]
	const T& Get() const noexcept { return m_baseStruct; }

private:
	T                                  m_baseStruct;
	std::shared_ptr<void>              m_lastInChain;
	std::vector<std::shared_ptr<void>> m_chainedStructs;

public:
	VkStructChain(const VkStructChain& other) noexcept
		: m_baseStruct{ other.m_baseStruct }, m_lastInChain{ other.m_lastInChain },
		m_chainedStructs{ other.m_chainedStructs }
	{}
	VkStructChain& operator=(const VkStructChain& other) noexcept
	{
		m_baseStruct     = other.m_baseStruct;
		m_lastInChain    = other.m_lastInChain;
		m_chainedStructs = other.m_chainedStructs;

		return *this;
	}
	VkStructChain(VkStructChain&& other) noexcept
		: m_baseStruct{ other.m_baseStruct }, m_lastInChain{ std::move(other.m_lastInChain) },
		m_chainedStructs{ std::move(other.m_chainedStructs) }
	{}
	VkStructChain& operator=(VkStructChain&& other) noexcept
	{
		m_baseStruct     = other.m_baseStruct;
		m_lastInChain    = std::move(other.m_lastInChain);
		m_chainedStructs = std::move(other.m_chainedStructs);

		return *this;
	}
};
#endif
