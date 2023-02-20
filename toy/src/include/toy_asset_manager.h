#pragma once

#include "toy_platform.h"
#include "toy_error.h"
#include "toy_memory.h"
#include "toy_asset.h"
#include "toy_file.h"



typedef struct toy_asset_manager_t {
	toy_memory_allocator_t* alc;
	toy_file_interface_t file_api;

	size_t cache_size;
	toy_memory_stack_p cache_stack;
	toy_allocator_t stack_alc_L;
	toy_allocator_t stack_alc_R;

	toy_memory_pool_p chunk_pools;
	toy_allocator_t chunk_alc;

	struct {
		toy_asset_pool_t node;
		toy_asset_pool_t camera;
		toy_asset_pool_t material;
		toy_asset_pool_t mesh_primitive;
		toy_asset_pool_t mesh;
		toy_asset_pool_t scene;
		//toy_asset_pool_t file;
		toy_asset_pool_t image;
		toy_asset_pool_t font;
	} asset_pools;

	toy_asset_item_ref_pool_t item_ref_pool;
	toy_asset_data_ref_pool_t data_ref_pool;

	void* mgr_private;
}toy_asset_manager_t;


TOY_EXTERN_C_START

void toy_create_asset_manager (
	size_t cache_size,
	toy_memory_allocator_t* alc,
	void* render_driver,
	toy_asset_manager_t* output,
	toy_error_t* error
);

void toy_destroy_asset_manager (
	toy_asset_manager_t* asset_mgr
);

void toy_load_mesh_primitive (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive_data,
	toy_asset_pool_item_ref_t* output,
	toy_error_t* error
);

TOY_EXTERN_C_END
