#pragma once

#include "toy_platform.h"
#include "toy_error.h"
#include "toy_memory.h"
#include "toy_asset.h"
#include "toy_file.h"

#include "platform/vulkan/toy_vulkan_asset.h"
#include "platform/vulkan/toy_vulkan_driver.h"
#include "platform/vulkan/toy_vulkan_asset_loader.h"


typedef struct toy_asset_manager_vulkan_private_t {
	toy_vulkan_driver_t* vk_driver;
	toy_vulkan_asset_loader_t vk_asset_loader;
	toy_vulkan_mesh_primitive_asset_pool_t vk_mesh_primitive_pool;
}toy_asset_manager_vulkan_private_t;


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
		toy_asset_pool_t image;
		toy_asset_pool_t image_sampler; // Todo: hash sample paremeters as key, and use hash table, add a default sampler
		toy_asset_pool_t material;
		toy_asset_pool_t mesh_primitive;
		toy_asset_pool_t mesh;
		toy_asset_pool_t scene;
		//toy_asset_pool_t file;
		toy_asset_pool_t font;
	} asset_pools;

	toy_asset_item_ref_pool_t item_ref_pool;
	toy_asset_data_ref_pool_t data_ref_pool;

	toy_asset_manager_vulkan_private_t vk_private;
}toy_asset_manager_t;


TOY_EXTERN_C_START

void toy_create_asset_manager (
	size_t cache_size,
	toy_memory_allocator_t* alc,
	toy_vulkan_driver_t* vk_driver,
	toy_asset_manager_t* output,
	toy_error_t* error
);

void toy_destroy_asset_manager (
	toy_asset_manager_t* asset_mgr
);

uint32_t toy_load_mesh_primitive (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive_data,
	toy_error_t* error
);

void toy_load_texture2d (
	toy_asset_manager_t* asset_mgr,
	const char* utf8_path,
	toy_asset_pool_item_ref_t* output,
	toy_error_t* error
);

uint32_t toy_alloc_material (
	toy_asset_manager_t* asset_mgr,
	size_t size,
	toy_error_t* error
);

toy_asset_pool_item_ref_t toy_create_image_sampler (
	toy_asset_manager_t* asset_mgr,
	toy_image_sampler_t* sampler_params,
	toy_error_t* error
);

TOY_EXTERN_C_END
