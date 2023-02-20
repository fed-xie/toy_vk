#pragma once

#include "toy_platform.h"
#include "toy_error.h"

TOY_EXTERN_C_START

enum toy_log_level {
	TOY_LOG_LEVEL_DEBUG = 0x1,
	TOY_LOG_LEVEL_INFO = 0x10,
	TOY_LOG_LEVEL_WARNING = 0x100,
	TOY_LOG_LEVEL_ERROR = 0x1000,
	TOY_LOG_LEVEL_FATAL = 0x10000,
};


void toy_log_message (enum toy_log_level lv, const char* fmt, ...);

#define toy_log_m(lv, fmt, ...) \
do { \
	toy_log_message(lv, fmt" %s %d\n", ##__VA_ARGS__, __FILE__, __LINE__); \
} while (0)

#define toy_log_f(fmt, ...) toy_log_m(TOY_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#define toy_log_e(fmt, ...) toy_log_m(TOY_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define toy_log_w(fmt, ...) toy_log_message(TOY_LOG_LEVEL_WARNING, fmt"\n", ##__VA_ARGS__)
#define toy_log_i(fmt, ...) toy_log_message(TOY_LOG_LEVEL_INFO, fmt"\n", ##__VA_ARGS__)
#define toy_log_d(fmt, ...) toy_log_m(TOY_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)


void toy_log_error (toy_error_t* error);

TOY_EXTERN_C_END
