#pragma once

#include "toy_platform.h"
#include "toy_error.h"

#include <stdio.h>
#include <stdint.h>


TOY_EXTERN_C_START

typedef void (*toy_open_file_fp)(
	void* ctx,
	const char* utf8_path,
	const char* mode,
	void* output,
	toy_error_t* error
);

typedef void (*toy_get_file_size_fp)(
	void* file,
	size_t* output,
	toy_error_t* error
);

typedef void (*toy_seek_file_fp)(
	void* file,
	size_t offset, // offset from beginning
	toy_error_t* error
);

typedef void (*toy_read_file_fp)(
	void* file,
	void* buffer,
	size_t buffer_size, // size to read
	size_t* output_byte, // size been read successfully
	toy_error_t* error
);

typedef void (*toy_close_file_fp)(void* file);

typedef struct toy_file_interface_t {
	void* context;
	size_t file_struct_size;
	toy_open_file_fp open_file;
	toy_get_file_size_fp get_file_size;
	toy_read_file_fp read_file;
	toy_close_file_fp close_file;
}toy_file_interface_t;


typedef struct toy_file_t {
	FILE* handle;
	size_t size; // file size
}toy_file_t;

void toy_open_file (
	void* _,
	const char* utf8_path,
	const char* mode,
	toy_file_t* output,
	toy_error_t* error
);

void toy_get_file_size (
	toy_file_t* file,
	size_t* output,
	toy_error_t* error
);

void toy_seek_file (
	toy_file_t* file,
	size_t offset, // offset from beginning
	toy_error_t* error
);

void toy_read_file (
	toy_file_t* file,
	void* buffer,
	size_t buffer_size, // size to read
	size_t* output_byte, // size been read successfully
	toy_error_t* error
);

void toy_close_file (toy_file_t* file);

// Standard file functions in stdio.h
toy_inline toy_file_interface_t toy_std_file_interface ()
{
	toy_file_interface_t std_file_intf;
	std_file_intf.context = NULL;
	std_file_intf.file_struct_size = sizeof(toy_file_t);
	std_file_intf.open_file = (toy_open_file_fp)toy_open_file;
	std_file_intf.get_file_size = (toy_get_file_size_fp)toy_get_file_size;
	std_file_intf.read_file = (toy_read_file_fp)toy_read_file;
	std_file_intf.close_file = (toy_close_file_fp)toy_close_file;
	return std_file_intf;
}

void toy_get_cwd (char* buffer, size_t buffer_size, size_t* output_size, toy_error_t* error);

void toy_set_cwd (const char* utf8_path, toy_error_t* error);

TOY_EXTERN_C_END


#include "toy_allocator.h"

TOY_EXTERN_C_START

// Return file content when success, NULL when failed
// MUST call toy_free_aligned(alc, file_content) after used
toy_aligned_p toy_std_load_whole_file (
	const char* utf8_path,
	const toy_allocator_t* alc,
	size_t* output_size,
	toy_error_t* error
);

// Return file content when success, NULL when failed
// MUST call toy_free_aligned(alc, file_content) after used
toy_aligned_p toy_load_whole_file (
	const char* utf8_path,
	const toy_file_interface_t* file_api,
	const toy_allocator_t* alc,
	const toy_allocator_t* tmp_alc,
	size_t* output_size,
	toy_error_t* error
);

TOY_EXTERN_C_END
