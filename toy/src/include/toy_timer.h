#pragma once

#include "toy_platform.h"

#include <stdint.h>

TOY_EXTERN_C_START

typedef struct toy_timer_t {
	union {
		uint64_t start_tick_u;
		int64_t start_tick_i;
	};
	union {
		uint64_t pause_tick_u;
		int64_t pause_tick_i;
	};
	uint64_t skip_ticks;
}toy_timer_t;

void toy_init_timer_env ();

void toy_sleep (uint32_t ms);

void toy_reset_timer (toy_timer_t* timer);

void toy_pause_timer (toy_timer_t* timer);
void toy_resume_timer (toy_timer_t* timer);

uint64_t toy_get_timer_during_ms (const toy_timer_t* timer);

TOY_EXTERN_C_END
