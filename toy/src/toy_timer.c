#include "include/toy_timer.h"


#ifdef TOY_OS_WINDOWS
#include <Windows.h>
static LARGE_INTEGER s_sys_time_freq;
#endif


#ifdef TOY_OS_WINDOWS
static inline LONGLONG toy_timer_get_time_tick () {
	LARGE_INTEGER performCount;
	QueryPerformanceCounter(&performCount);
	return performCount.QuadPart;
}
#endif


void toy_init_timer_env () {
#ifdef TOY_OS_WINDOWS
	QueryPerformanceFrequency(&s_sys_time_freq);
#endif
}


void toy_sleep (uint32_t ms) {
#ifdef TOY_OS_WINDOWS
	Sleep((DWORD)ms);
#else
#error "Unsupported OS"
#endif // TOY_OS_WINDOWS
}


void toy_reset_timer (toy_timer_t* timer) {
#ifdef TOY_OS_WINDOWS
	LONGLONG tick = toy_timer_get_time_tick();
	timer->start_tick_i = tick;
	timer->skip_ticks = 0;
	timer->pause_tick_i = 0;
#else
#error "Unsupported OS for timer"
#endif
}


void toy_pause_timer (toy_timer_t* timer) {
#ifdef TOY_OS_WINDOWS
	LONGLONG tick = toy_timer_get_time_tick();
	timer->pause_tick_i = tick;
#else
#error "Unsupported OS for timer"
#endif
}


void toy_resume_timer (toy_timer_t* timer) {
#ifdef TOY_OS_WINDOWS
	LONGLONG tick = toy_timer_get_time_tick();
	timer->skip_ticks = tick - timer->pause_tick_i;
#else
#error "Unsupported OS for timer"
#endif
}


uint64_t toy_get_timer_during_ms (const toy_timer_t* timer) {
#ifdef TOY_OS_WINDOWS
	LONGLONG tick = toy_timer_get_time_tick();
	return (tick - timer->start_tick_i - timer->skip_ticks) * 1000LL / s_sys_time_freq.QuadPart;
#else
#error "Unsupported OS for timer"
	// The tick may overflow
	if (tick > timer->start_tick_u)
		return (tick - timer->start_tick_u - timer->skip_ticks) * 1000LL / s_sys_time_freq.QuadPart;
	else
		return (timer->start_tick_u - tick - timer->skip_ticks) * 1000LL / s_sys_time_freq.QuadPart;
#endif
}
