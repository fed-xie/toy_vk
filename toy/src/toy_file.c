#include "include/toy_file.h"

#if TOY_OS_WINDOWS
#include <Windows.h>
#endif

#include <errno.h>
#include "include/toy_log.h"
#include "toy_assert.h"

void toy_open_file (
	void* _,
	const char* utf8_path,
	const char* mode,
	toy_file_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != output);
	TOY_ASSERT(NULL != error);
	errno_t err = 0;

	FILE* file = NULL;
	err = fopen_s(&file, utf8_path, mode);
	if (0 != err) {
		toy_log_e("Open file %s failed", utf8_path);
		toy_err_errno(TOY_ERROR_FILE_OPEN_FAILED, err, "fopen_s failed", error);
		return;
	}

	TOY_ASSERT(NULL != file);

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	rewind(file);

	output->handle = file;
	output->size = file_size;
	toy_ok(error);
}


void toy_get_file_size (
	toy_file_t* file,
	size_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);

	if (toy_unlikely(NULL == file)) {
		toy_err(TOY_ERROR_NULL_INPUT, "NULL file", error);
		return;
	}

	*output = file->size;
	toy_ok(error);
}


void toy_seek_file (
	toy_file_t* file,
	size_t offset, // offset from beginning
	toy_error_t* error)
{
	TOY_ASSERT(NULL != error);

	int err = fseek(file->handle, (long)offset, SEEK_SET);
	if (toy_unlikely(0 != err))
		toy_err_int(TOY_ERROR_FILE_SEEK_FAILED, err, "fseek failed", error);
	else
		toy_ok(error);
}


void toy_read_file (
	toy_file_t* file,
	void* buffer,
	size_t buffer_size, // size to read
	size_t* output_byte, // size been read successfully
	toy_error_t* error)
{
	TOY_ASSERT(NULL != file && NULL != file->handle);
	TOY_ASSERT(NULL != error);

	if (0 == buffer_size || 0 == file->size) {
		if (NULL != output_byte)
			*output_byte = 0;
		toy_ok(error);
		return;
	}

	size_t size_read = fread(buffer, 1, buffer_size, file->handle);
	if (size_read < buffer_size) {
		int err = ferror(file->handle);
		if (0 != err) {
			if (NULL != output_byte)
				*output_byte = 0;
			toy_err_int(TOY_ERROR_FILE_READ_FAILED, err, "fread failed", error);
			return;
		}
	}

	if (NULL != output_byte)
		*output_byte = size_read;
	toy_ok(error);
}


void toy_close_file (toy_file_t* file) {
	if (NULL != file) {
		if (NULL != file->handle) {
			fclose(file->handle);
			file->handle = NULL;
		}
		file->size = 0;
	}
}


void toy_get_cwd (char* buffer, size_t buffer_size, size_t* output_size, toy_error_t* error)
{
#if TOY_OS_WINDOWS
	TCHAR path_buffer[MAX_PATH];
	// Multithreaded applications and shared library code should not use the
	// GetCurrentDirectory functionand should avoid using relative path names.
	DWORD path_slen = GetCurrentDirectoryW(MAX_PATH, path_buffer);
	if (0 == path_slen) {
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, GetLastError(), "GetCurrentDirectoryW failed", error);
		return;
	}

	int byte_cvted = WideCharToMultiByte(CP_UTF8, 0, path_buffer, -1, buffer, buffer_size, NULL, NULL);
	if (0 == byte_cvted) {
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, GetLastError(), "WideCharToMultiByte failed", error);
		return;
	}

	*output_size = byte_cvted;
	toy_ok(error);
	return;
#else
#error "Unsupported OS"
#endif
}


void toy_set_cwd (const char* utf8_path, toy_error_t* error)
{
#if TOY_OS_WINDOWS
	WCHAR path_buffer[MAX_PATH];

	int numUtf16Cvted = MultiByteToWideChar(
		CP_UTF8, 0, utf8_path, -1,
		path_buffer, MAX_PATH);
	if (0 == numUtf16Cvted) {
		toy_err_dword(TOY_ERROR_ASSERT_FAILED, GetLastError(), "MultiByteToWideChar failed", error);
		return;
	}

	BOOL success = SetCurrentDirectoryW(path_buffer);
	if (success)
		toy_ok(error);
	else
		toy_err_dword(TOY_ERROR_OPERATION_FAILED, GetLastError(), "SetCurrentDirectoryW failed", error);
#else
#error "Unsupported OS"
#endif
}


toy_aligned_p toy_std_load_whole_file (
	const char* utf8_path,
	const toy_allocator_t* alc,
	size_t* output_size,
	toy_error_t* error)
{
	TOY_ASSERT(NULL != utf8_path);
	TOY_ASSERT(NULL != alc);
	TOY_ASSERT(NULL != output_size);
	TOY_ASSERT(NULL != error);

	toy_file_t entry_file;
	toy_open_file(NULL, utf8_path, "r", &entry_file, error);
	if (toy_is_failed(*error)) {
		return NULL;
	}

	size_t file_size = 0;
	toy_get_file_size(&entry_file, &file_size, error);
	if (toy_is_failed(*error)) {
		toy_close_file(&entry_file);
		return NULL;
	}

	toy_aligned_p buffer = toy_alloc_aligned(alc, file_size, sizeof(void*));
	if (toy_unlikely(NULL == buffer)) {
		toy_close_file(&entry_file);
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to alloc to read file", error);
		return NULL;
	}

	toy_read_file(&entry_file, buffer, file_size, output_size, error);
	if (toy_is_failed(*error)) {
		toy_free_aligned(alc, buffer);
		toy_close_file(&entry_file);
		return NULL;
	}

	toy_close_file(&entry_file);

	toy_ok(error);
	return buffer;
}


toy_aligned_p toy_load_whole_file (
	const char* utf8_path,
	const toy_file_interface_t* file_api,
	const toy_allocator_t* alc,
	const toy_allocator_t* tmp_alc,
	size_t* output_size,
	toy_error_t* error)
{
	toy_aligned_p file = toy_alloc_aligned(tmp_alc, file_api->file_struct_size, sizeof(void*));
	if (NULL == file) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc file structure failed", error);
		goto FAIL_ALLOC_FILE_STRUCT;
	}

	file_api->open_file(file_api->context, utf8_path, "rb", file, error);
	if (toy_is_failed(*error))
		goto FAIL_OPEN;

	size_t file_size = 0;
	file_api->get_file_size(file, &file_size, error);
	if (toy_unlikely(toy_is_failed(*error)))
		goto FAIL_GET_SIZE;

	toy_aligned_p file_content = toy_alloc_aligned(alc, file_size, sizeof(uint64_t));
	if (NULL == file_content) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Alloc file content failed", error);
		goto FAIL_ALLOC_CONTENT;
	}

	size_t size_read = 0;
	file_api->read_file(file, file_content, file_size, &size_read, error);
	if (toy_is_failed(*error))
		goto FAIL_READ;

	file_api->close_file(file);
	toy_free_aligned(tmp_alc, file);

	if (NULL != output_size)
		*output_size = size_read;

	toy_ok(error);
	return file_content;

FAIL_READ:
	toy_free_aligned(alc, file_content);
FAIL_ALLOC_CONTENT:
FAIL_GET_SIZE:
	file_api->close_file(file);
FAIL_OPEN:
	toy_free_aligned(tmp_alc, file);
FAIL_ALLOC_FILE_STRUCT:
	return NULL;
}
