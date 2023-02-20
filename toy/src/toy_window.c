#include "include/toy_window.h"

#if TOY_OS_WINDOWS
#include "platform/windows/toy_win_window.h"
#endif


void toy_create_window (
	const toy_window_create_info_t* window_ci,
	toy_window_t* output,
	toy_error_t* error)
{
	output->hid = window_ci->hid;
#if TOY_OS_WINDOWS
	toy_create_win_window(window_ci, output, error);
#else
#error "Unsupported OS"
#endif
}


void toy_destroy_window (toy_window_t* window, toy_error_t* error)
{
#if TOY_OS_WINDOWS
	toy_destroy_win_window(window, error);
#else
#error "Unsupported OS"
#endif
}


void toy_query_window_messages (toy_window_t* window, toy_error_t* error)
{
#if TOY_OS_WINDOWS
	toy_query_win_window_messages(window);
	toy_ok(error);
#else
#error "Unsupported OS"
#endif
}


void toy_set_window_title (toy_window_t* window, const char* utf8_title, toy_error_t* error)
{
#if TOY_OS_WINDOWS
	toy_set_win_window_title(window, utf8_title, error);
#else
#error "Unsupported OS"
#endif
}
