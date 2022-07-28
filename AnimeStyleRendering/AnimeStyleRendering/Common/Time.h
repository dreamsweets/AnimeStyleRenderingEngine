#pragma once
#include "Common/HighResolutionClock.h"

class Time {
public:
	static float GetDeltaTime();
	static float GetTotalTime();
	
	static void Init();
	static void Tick();
};