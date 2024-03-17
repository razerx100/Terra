#ifndef TERRA_EVENTS_HPP_
#define TERRA_EVENTS_HPP_
#include <TitanDispatcher.hpp>

enum class TerraEventType
{
	QueueExecutionFinished,
	None
};

using TerraDispatcher = TitanDispatcher<TerraEventType>;

class QueueExecutionFinishedEvent : public FeedbackEvent<TerraEventType>
{
public:
	QueueExecutionFinishedEvent(std::uint8_t queueIndex) : m_queueIndex{ queueIndex } {}

	[[nodiscard]]
	TerraEventType GetType() const noexcept override { return TerraEventType::QueueExecutionFinished; }
	[[nodiscard]]
	std::uint8_t QueueIndex() const noexcept { return m_queueIndex; }

private:
	std::uint8_t m_queueIndex;
};
#endif
