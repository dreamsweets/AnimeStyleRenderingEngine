#include "Time.h"

namespace {
HighResolutionClock* m_pTimer = nullptr;
}

float Time::GetDeltaTime()
{
	return (float)m_pTimer->GetDeltaSeconds();
}

float Time::GetTotalTime()
{
	return (float)m_pTimer->GetTotalSeconds();
}

void Time::Init()
{
	m_pTimer = new HighResolutionClock;
}

void Time::Tick()
{
	m_pTimer->Tick();
}