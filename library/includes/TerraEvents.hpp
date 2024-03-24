#ifndef TERRA_EVENTS_HPP_
#define TERRA_EVENTS_HPP_
#include <TitanDispatcher.hpp>

enum class TerraEventType
{
	GfxBufferExecutionFinished,
	GfxQueueExecutionFinished,
	InterruptGfxQueue,
	None
};

using TerraDispatcher = TitanDispatcher<TerraEventType>;
using TerraEvent      = TitanEvent<TerraEventType>;

class GfxBufferExecutionFinishedEvent : public FeedbackEvent<TerraEventType>
{
public:
	GfxBufferExecutionFinishedEvent(std::uint8_t queueIndex) : m_queueIndex{ queueIndex } {}

	[[nodiscard]]
	TerraEventType GetType() const noexcept override
	{
		return TerraEventType::GfxBufferExecutionFinished;
	}
	[[nodiscard]]
	std::uint8_t QueueIndex() const noexcept { return m_queueIndex; }

private:
	std::uint8_t m_queueIndex;
};

class GfxQueueExecutionFinishedEvent : public FeedbackEvent<TerraEventType>
{
public:
	[[nodiscard]]
	TerraEventType GetType() const noexcept override
	{
		return TerraEventType::GfxQueueExecutionFinished;
	}
};

class InterruptGfxQueueEvent : public TitanEvent<TerraEventType>
{
public:
	[[nodiscard]]
	TerraEventType GetType() const noexcept override
	{
		return TerraEventType::InterruptGfxQueue;
	}
};
#endif
