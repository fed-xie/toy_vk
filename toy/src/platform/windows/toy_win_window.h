#pragma once

#include "../../include/toy_platform.h"

#include "../../include/toy_window.h"

TOY_EXTERN_C_START

void toy_create_win_window (
	const toy_window_create_info_t* window_ci,
	toy_window_t* output,
	toy_error_t* error
);

void toy_destroy_win_window (toy_window_t* window, toy_error_t* error);

void toy_query_win_window_messages (toy_window_t* window);

void toy_set_win_window_title (toy_window_t* window, const char* utf8_title, toy_error_t* error);

TOY_EXTERN_C_END
