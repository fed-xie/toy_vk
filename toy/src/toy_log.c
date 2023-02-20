#include "include/toy_log.h"

#include <stdio.h>
#include <stdarg.h>

/*
    Todo:
    #include <execinfo.h> and print backtrace in gcc
    Or use boost.stacktrace
*/

#ifdef __linux__
#include <execinfo.h>
#endif


void toy_log_message (enum toy_log_level lv, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    fflush(stdout);
}


void toy_log_error (toy_error_t* error)
{
    const char* msg = "";
    if (NULL != error->error_msg)
        msg = error->error_msg;

    switch (error->inner_error_type)
    {
    case TOY_ERROR_TYPE_TOY:
        toy_log_message(TOY_LOG_LEVEL_ERROR, "ERROR_TOY: %s (%"PRIi32") %s(%d)\n", msg, error->inner_error.toy, error->file, error->line);
        return;
    case TOY_ERROR_TYPE_INT:
        toy_log_message(TOY_LOG_LEVEL_ERROR, "ERROR_INT: %s (%d) %s(%d)\n", msg, error->inner_error.int_err, error->file, error->line);
        return;
    case TOY_ERROR_TYPE_ERRNO:
        toy_log_message(TOY_LOG_LEVEL_ERROR, "ERROR_ERRNO: %s (%d) %s(%d)\n", msg, error->inner_error.err_no, error->file, error->line);
        return;
    case TOY_ERROR_TYPE_VK_ERROR:
        toy_log_message(TOY_LOG_LEVEL_ERROR, "ERROR_VK: %s (%d) %s(%d)\n", msg, error->inner_error.vk_err, error->file, error->line);
        return;
    case TOY_ERROR_TYPE_WINDOWS_DWORD:
        toy_log_message(TOY_LOG_LEVEL_ERROR, "ERROR_DWORD: %s (%ld) %s(%d)\n", msg, error->inner_error.dw_err, error->file, error->line);
        return;
    default:
        toy_log_message(TOY_LOG_LEVEL_ERROR, "ERROR_UNKNOWN: %s %s(%d)\n", msg, error->file, error->line);
        return;
    }
}
