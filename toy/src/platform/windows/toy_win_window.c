#include "toy_win_window.h"

#include "../../include/toy_log.h"
#include "../../toy_assert.h"

// Windows functions end with 'A' means ANSI character set, end with 'W' means Unicode(UTF-16)

#ifdef UNICODE
static const LPCWSTR s_toy_win_window_class_name = TEXT("toy_win_window_class_name");
#else
static const LPCSTR s_toy_win_window_class_name = TEXT("toy_win_window_class_name");
#endif


#include <windowsx.h>

// mouse input messages: https://docs.microsoft.com/en-us/windows/win32/inputdev/mouse-input-notifications
// keyboard input messages: https://docs.microsoft.com/en-us/windows/win32/inputdev/keyboard-input-notifications
#define TOY_MSG_RESULT_HANDLED 0
#define TOY_MSG_RESULT_DEFAULT 1
#define TOY_MSG_RESULT_IGNORE -1
#define TOY_MSG_RESULT_UNKNOWN -2
static LRESULT toy_process_hid_input (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LONG_PTR lPtr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
	toy_window_t* toy_window = (toy_window_t*)lPtr;
	if (NULL == toy_window || NULL == toy_window->hid)
		return TOY_MSG_RESULT_IGNORE;

	toy_mouse_t* mouse = &toy_window->hid->mouse;
	toy_keyboard_t* keyboard = &toy_window->hid->keyboard;
	switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		toy_set_keyboard_key(keyboard, (uint8_t)wParam, TOY_KEY_STATE_DOWN);
		return TOY_MSG_RESULT_DEFAULT;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		toy_set_keyboard_key(keyboard, (uint8_t)wParam, TOY_KEY_STATE_UP);
		return TOY_MSG_RESULT_DEFAULT;
	case WM_LBUTTONUP:
		toy_set_mouse_key(mouse, TOY_MOUSE_KEY_LEFT, TOY_KEY_STATE_UP);
		ReleaseCapture();
		return TOY_MSG_RESULT_DEFAULT;
	case WM_RBUTTONUP:
		toy_set_mouse_key(mouse, TOY_MOUSE_KEY_RIGHT, TOY_KEY_STATE_UP);
		ReleaseCapture();
		return TOY_MSG_RESULT_DEFAULT;
	case WM_MBUTTONUP:
		toy_set_mouse_key(mouse, TOY_MOUSE_KEY_MIDDLE, TOY_KEY_STATE_UP);
		ReleaseCapture();
		return TOY_MSG_RESULT_DEFAULT;
	case WM_LBUTTONDOWN:
		SetCapture(hWnd);
		toy_set_mouse_key(mouse, TOY_MOUSE_KEY_LEFT, TOY_KEY_STATE_DOWN);
		return TOY_MSG_RESULT_DEFAULT;
	case WM_RBUTTONDOWN:
		SetCapture(hWnd);
		toy_set_mouse_key(mouse, TOY_MOUSE_KEY_RIGHT, TOY_KEY_STATE_DOWN);
		return TOY_MSG_RESULT_DEFAULT;
	case WM_MBUTTONDOWN:
		SetCapture(hWnd);
		toy_set_mouse_key(mouse, TOY_MOUSE_KEY_MIDDLE, TOY_KEY_STATE_DOWN);
		return TOY_MSG_RESULT_DEFAULT;
	case WM_MOUSEMOVE:
		toy_set_mouse_position(mouse, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return TOY_MSG_RESULT_DEFAULT;
	case WM_MOUSEWHEEL:
		toy_set_mouse_roll_steps(mouse, GET_WHEEL_DELTA_WPARAM(wParam) / -WHEEL_DELTA);
		return TOY_MSG_RESULT_DEFAULT;
	case WM_MOUSELEAVE:
		return TOY_MSG_RESULT_DEFAULT;
	default:
		toy_log_w("Unknown window HID message %u", (unsigned int)uMsg);
	}
	return TOY_MSG_RESULT_UNKNOWN;
}


// https://docs.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues#system-defined-messages
static LRESULT CALLBACK toy_win_window_proc(
	_In_ HWND   hWnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	//toy_log_d("receive window msg %x\n", uMsg);
	switch (uMsg) {
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_MOUSEMOVE:
	case WM_MOUSEWHEEL:
	case WM_MOUSELEAVE:
		if (TOY_MSG_RESULT_HANDLED == toy_process_hid_input(hWnd, uMsg, wParam, lParam))
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0); // Only send WM_QUIT with WM_DESTROY
		return 0;
	case WM_SIZE:
	{
		LONG_PTR lPtr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
		UINT width = LOWORD(lParam);
		UINT height = HIWORD(lParam);
		toy_window_t* toy_window = (toy_window_t*)lPtr;
		if (NULL != toy_window) {
			toy_window->width = width;
			toy_window->height = height;
		}
		break;
	}
	case WM_CLOSE:
	{
		// Todo: confirm for closing action
		LONG_PTR lPtr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
		toy_window_t* toy_window = (toy_window_t*)lPtr;
		if (NULL != toy_window) {
			if (NULL != toy_window->os_particular.hWnd) {
				DestroyWindow(toy_window->os_particular.hWnd);
				toy_window->os_particular.hWnd = NULL;
				return 0;
			}
		}
		break;
	}
	//case WM_SIZING:
	case WM_SHOWWINDOW:
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void toy_create_win_window (
	const toy_window_create_info_t* window_ci,
	toy_window_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;// | WS_THICKFRAME;
	DWORD ex_style = WS_EX_ACCEPTFILES | WS_EX_APPWINDOW |
		WS_EX_LEFT | WS_EX_OVERLAPPEDWINDOW;

	RECT wnd_size;
	wnd_size.left = wnd_size.top = 0;
	wnd_size.right = window_ci->width;
	wnd_size.bottom = window_ci->height;
	BOOL adjust_success = AdjustWindowRectEx(&wnd_size, style, FALSE, ex_style);
	if (!adjust_success) {
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, GetLastError(), "AdjustWindowRectEx failed", error);
		return;
	}

	WNDCLASSEX wnd_class;
	wnd_class.cbSize = sizeof(WNDCLASSEX);
	wnd_class.style = CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW |
		CS_OWNDC | CS_PARENTDC |
		CS_VREDRAW | CS_HREDRAW;
	wnd_class.lpfnWndProc = toy_win_window_proc;
	wnd_class.cbClsExtra = 0;
	wnd_class.cbWndExtra = 0;
	wnd_class.hInstance = window_ci->hInst;
	wnd_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wnd_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd_class.hbrBackground = (HBRUSH)GetStockObject(COLOR_BACKGROUND);
	wnd_class.lpszMenuName = NULL;
	wnd_class.lpszClassName = s_toy_win_window_class_name;
	wnd_class.hIconSm = NULL; // Small icon

	ATOM class_id = RegisterClassEx(&wnd_class);
	if (0 == class_id) {
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, GetLastError(), "RegisterClassEx failed", error);
		return;
	}


#if UNICODE
	LPCWSTR title = NULL;
	const char* ci_title = (NULL == window_ci->title) ? "" : window_ci->title;
	WCHAR* wchar_buffer = output->os_particular.title_buffer.wchar_buffer;
	const int buffer_length = sizeof(output->os_particular.title_buffer.wchar_buffer) / sizeof(WCHAR);
	int numUtf16Cvted = MultiByteToWideChar(
		CP_UTF8, 0, ci_title, -1,
		wchar_buffer, buffer_length);
	if (0 == numUtf16Cvted) {
		toy_err_dword(TOY_ERROR_ASSERT_FAILED, GetLastError(), "MultiByteToWideChar failed", error);
		return;
	}
	title = (LPCWSTR)wchar_buffer;
#else
	LPCSTR title = NULL;
	const char* ci_title = (NULL == window_ci->title) ? "" : window_ci->title;
	CHAR* char_buffer = output->os_particular.title_buffer.char_buffer;
	const int buffer_length = sizeof(output->os_particular.title_buffer.char_buffer);
	strncpy_s(char_buffer, buffer_length, ci_title, buffer_length - 1);
	char_buffer[buffer_length - 1] = '\0';
	title = (LPARAM)char_buffer;
#endif

	HWND hWnd = CreateWindowEx(
		ex_style,
		s_toy_win_window_class_name,
		title,
		style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		wnd_size.right - wnd_size.left,
		wnd_size.bottom - wnd_size.top,
		NULL,
		NULL,
		window_ci->hInst,
		NULL);
	if (NULL == hWnd) {
		DWORD dw_err = GetLastError();
		toy_log_d("Failed to craete win window %lu\n", dw_err);
		if (0 == UnregisterClass(s_toy_win_window_class_name, window_ci->hInst)) {
			DWORD dw_err = GetLastError();
			toy_log_d("Failed to unregister window class %lu", dw_err);
		}
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, dw_err, "CreateWindowEx failed", error);
		return;
	}

	// Center the window
	// GetMonitorInfo()
	RECT workArea;
	BOOL isSuccess = SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	if (!isSuccess) {
		DWORD dw_err = GetLastError();
		toy_log_w("Failed to get screen work area size %lu", dw_err);
	}
	else {
		int win_x = ((workArea.right - workArea.left) - (wnd_size.right - wnd_size.left)) / 2;
		int win_y = ((workArea.bottom - workArea.top) - (wnd_size.bottom - wnd_size.top)) / 2;
		win_x = win_x < 0 ? 0 : win_x;
		win_y = win_y < 0 ? 0 : win_y;
		isSuccess = SetWindowPos(hWnd, NULL, win_x, win_y, wnd_size.right - wnd_size.left, wnd_size.bottom - wnd_size.top, 0);
		if (!isSuccess) {
			DWORD dw_err = GetLastError();
			toy_log_w("Failed to set window center", dw_err);
		}
	}
	//GetSystemMetrics(SM_CXFULLSCREEN);

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);

	output->width = window_ci->width;
	output->height = window_ci->height;
	output->os_particular.hInst = window_ci->hInst;
	output->os_particular.hWnd = hWnd;
	output->os_particular.rect = wnd_size;
	output->is_quit = 0;

	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)output);

	toy_ok(error);
}


void toy_destroy_win_window (
	toy_window_t* window,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);
	TOY_ASSERT(NULL != window);

	if (NULL != window->os_particular.hWnd) {
		BOOL ret = DestroyWindow(window->os_particular.hWnd);
		if (!ret) {
			DWORD dw_err = GetLastError();
			toy_log_d("Failed to destroy window: %lu\n", dw_err);
			toy_err_dword(TOY_ERROR_OPERATION_FAILED, dw_err, "DestroyWindow failed", error);
		}

		// Clear left messages
		MSG msg;
		while (PeekMessage(&msg, window->os_particular.hWnd, 0, 0, PM_REMOVE)) {
			if (WM_QUIT == msg.message) {
				window->is_quit = 1;
				//*hWnd = NULL; // Only receive WM_QUIT with WM_DESTROY
				break;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		while (PeekMessage(&msg, (HWND)-1, 0, 0, PM_REMOVE)) {
			if (WM_QUIT == msg.message) {
				window->is_quit = 1;
				//*hWnd = NULL; // Only receive WM_QUIT with WM_DESTROY
				break;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		window->os_particular.hWnd = NULL;
	}

	if (0 == UnregisterClass(s_toy_win_window_class_name, window->os_particular.hInst)) {
		DWORD dw_err = GetLastError();
		toy_log_d("Failed to unregister window class %lu\n", dw_err);
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, dw_err, "DestroyWindow failed", error);
	}

	toy_ok(error);
}


void toy_query_win_window_messages (toy_window_t* window) {
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
		if (WM_QUIT == msg.message) {
			window->is_quit = 1;
			//*hWnd = NULL; // Only receive WM_QUIT with WM_DESTROY
			break;
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}


void toy_set_win_window_title (toy_window_t* window, const char* utf8_title, toy_error_t* error) {
	LPARAM title = 0;
#if UNICODE
	WCHAR* wchar_buffer = window->os_particular.title_buffer.wchar_buffer;
	const int buffer_length = sizeof(window->os_particular.title_buffer.wchar_buffer) / sizeof(WCHAR);
	int numUtf16Cvted = MultiByteToWideChar(
		CP_UTF8, 0, utf8_title, -1,
		wchar_buffer, buffer_length);
	if (0 == numUtf16Cvted) {
		toy_err_dword(TOY_ERROR_ASSERT_FAILED, GetLastError(), "MultiByteToWideChar failed", error);
		return;
	}
	title = (LPARAM)wchar_buffer;
#else
	CHAR* char_buffer = window->os_particular.title_buffer.char_buffer;
	const int buffer_length = sizeof(window->os_particular.title_buffer.char_buffer);
	strncpy_s(char_buffer, buffer_length, utf8_title, buffer_length - 1);
	char_buffer[buffer_length - 1] = '\0';
	title = (LPARAM)char_buffer;
#endif
	//SetWindowText(hWnd, utf8_title);

	UINT timeout = 100; // milliseconds
	DWORD_PTR dw_result;
	LRESULT result = SendMessageTimeout(window->os_particular.hWnd, WM_SETTEXT,
		0, title,
		SMTO_ABORTIFHUNG, timeout, &dw_result);
	if (0 == result) {
		DWORD dw_err = GetLastError();
		toy_log_d("Set window title failed %lu\n", dw_err);
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, dw_err, "SendMessageTimeout WM_SETTEXT failed", error);
		return;
	}

	toy_ok(error);
}
