#pragma once

#include "toy_platform.h"
#include "toy_error.h"
#include "toy_hid.h"
#if TOY_OS_WINDOWS
#include <Windows.h>
#endif

TOY_EXTERN_C_START

typedef struct toy_window_create_info_t {
#if TOY_OS_WINDOWS
	HINSTANCE hInst;
#endif
	int width;
	int height;
	toy_hid_t* hid;
	const char* title;
}toy_window_create_info_t;


typedef struct toy_window_t {
	int width;
	int height;
	toy_hid_t* hid;
	int is_quit;
#if TOY_OS_WINDOWS
	struct {
		RECT rect;
		union {
			WCHAR wchar_buffer[1024];
			char char_buffer[1024];
		} title_buffer;
		HINSTANCE hInst;
		HWND hWnd;
	} os_particular;
#endif
}toy_window_t;


void toy_create_window (
	const toy_window_create_info_t* window_ci,
	toy_window_t* output,
	toy_error_t* error
);

void toy_destroy_window (toy_window_t* window, toy_error_t* error);

void toy_query_window_messages (toy_window_t* window, toy_error_t* error);

void toy_set_window_title (toy_window_t* window, const char* utf8_title, toy_error_t* error);

TOY_EXTERN_C_END
