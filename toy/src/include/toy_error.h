#pragma once

#include "toy_platform.h"
#include <inttypes.h>
#include <stdbool.h>


TOY_EXTERN_C_START

enum toy_error_type_t {
	TOY_ERROR_TYPE_TOY = 0,
	TOY_ERROR_TYPE_INT,
	TOY_ERROR_TYPE_ERRNO,
	TOY_ERROR_TYPE_VK_ERROR,
	TOY_ERROR_TYPE_WINDOWS_DWORD,
	TOY_ERROR_TYPE_MAX = 0x7fffffff,
};

typedef struct toy_error_t {
	int32_t err_code;
	enum toy_error_type_t inner_error_type;
	union {
		int32_t toy;
		errno_t err_no;
		int32_t vk_err;
		int int_err;
#if TOY_OS_WINDOWS
		unsigned long dw_err;
#endif
	} inner_error;
	const char* file;
	int line;
	const char* error_msg;
}toy_error_t;


#define toy_err(code, msg, err_p) \
do { \
	(err_p)->err_code = (code);\
	(err_p)->inner_error_type = TOY_ERROR_TYPE_TOY;\
	(err_p)->inner_error.toy = (code);\
	(err_p)->error_msg = (msg);\
	(err_p)->file = __FILE__;\
	(err_p)->line = __LINE__;\
} while (false)


#define toy_err_errno(toy_code, err_no_code, msg, err_p) \
do { \
	(err_p)->err_code = (toy_code);\
	(err_p)->inner_error_type = TOY_ERROR_TYPE_ERRNO;\
	(err_p)->inner_error.toy = (err_no_code);\
	(err_p)->error_msg = (msg);\
	(err_p)->file = __FILE__;\
	(err_p)->line = __LINE__;\
} while (false)


#define toy_err_int(toy_code, int_err_code, msg, err_p) \
do { \
	(err_p)->err_code = (toy_code);\
	(err_p)->inner_error_type = TOY_ERROR_TYPE_INT;\
	(err_p)->inner_error.toy = (int_err_code);\
	(err_p)->error_msg = (msg);\
	(err_p)->file = __FILE__;\
	(err_p)->line = __LINE__;\
} while (false)


#define toy_err_vkerr(toy_code, vk_err_code, msg, err_p) \
do { \
	(err_p)->err_code = (toy_code);\
	(err_p)->inner_error_type = TOY_ERROR_TYPE_VK_ERROR;\
	(err_p)->inner_error.toy = (vk_err_code);\
	(err_p)->error_msg = (msg);\
	(err_p)->file = __FILE__;\
	(err_p)->line = __LINE__;\
} while (false)


#if TOY_OS_WINDOWS
#define toy_err_dword(toy_code, dw_err_code, msg, err_p) \
do { \
	(err_p)->err_code = (toy_code);\
	(err_p)->inner_error_type = TOY_ERROR_TYPE_WINDOWS_DWORD;\
	(err_p)->inner_error.toy = (dw_err_code);\
	(err_p)->error_msg = (msg);\
	(err_p)->file = __FILE__;\
	(err_p)->line = __LINE__;\
} while (false)
#endif


enum toy_error_enum_t {
	TOY_ERROR_OK = 0,
	TOY_ERROR_TIMEOUT,
	TOY_ERROR_BUSY,
	TOY_ERROR_UNKNOWN_ERROR,
	TOY_ERROR_OPERATION_FAILED, // Don't use it if allowed
	TOY_ERROR_CREATE_OBJECT_FAILED,
	TOY_ERROR_OBJECT_NOT_EXIST,
	TOY_ERROR_NULL_INPUT,
	TOY_ERROR_ASSERT_FAILED,
	TOY_ERROR_FILE_NOT_FOUND,
	TOY_ERROR_FILE_OPEN_FAILED,
	TOY_ERROR_FILE_READ_FAILED,
	TOY_ERROR_FILE_SEEK_FAILED,
	TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED,
	TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED,
	TOY_ERROR_MEMORY_ALIGNMENT_ERROR,
	TOY_ERROR_MEMORY_MAPPING_FAILED,
	TOY_ERROR_MEMORY_FLUSH_FAILED,
	TOY_ERROR_RENDER_RECORD_CMD_FAILED,
};


toy_inline void toy_no_err (toy_error_t* e, const char* file, int line) {
	e->err_code = TOY_ERROR_OK;
	e->inner_error_type = TOY_ERROR_TYPE_TOY;
	e->inner_error.toy = 0;
	e->error_msg = "";
	e->file = file;
	e->line = line;
}

#define toy_ok(err_p) toy_no_err(err_p, __FILE__, __LINE__)

toy_inline bool toy_is_ok (toy_error_t err) {
	return TOY_ERROR_OK == err.err_code;
}

toy_inline bool toy_is_failed (toy_error_t err) {
	return TOY_ERROR_OK != err.err_code;
}


TOY_EXTERN_C_END
