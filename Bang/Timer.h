struct Timer
{
	float current_tick;
	float threshold;

	Timer() 
	{
		current_tick = 0;
		threshold = 0;
	}

	Timer(float pSeconds)
	{
		current_tick = 0;
		threshold = pSeconds;
	}
};

static inline bool TickTimer(Timer* t, float pDeltaTime)
{
	t->current_tick += pDeltaTime;
	if (t->current_tick >= t->threshold)
	{
		t->current_tick -= t->threshold;
		return true;
	}

	return false;
}

static inline void ResetTimer(Timer* t, float pDuration)
{
	t->current_tick = 0;
	t->threshold = pDuration;
}

static void SleepUntilFrameEnd(__int64 pStart, double pFrequency)
{
	LARGE_INTEGER query_counter;
	QueryPerformanceCounter(&query_counter);
	double update_seconds = double(query_counter.QuadPart - pStart) / pFrequency;

	if (update_seconds < ExpectedSecondsPerFrame)
	{
		int sleep_time = (int)((ExpectedSecondsPerFrame - update_seconds) * 1000);
		Sleep(sleep_time);
	}
}