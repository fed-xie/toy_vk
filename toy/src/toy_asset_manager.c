#include "include/toy_asset_manager.h"

#include "toy_assert.h"
#include "include/platform/vulkan/toy_vulkan_driver.h"
#include "include/platform/vulkan/toy_vulkan_asset.h"
#include "platform/vulkan/toy_vulkan_asset_loader.h"
#include <string.h>


typedef struct toy_asset_manager_vulkan_private_t {
	toy_vulkan_driver_t* vk_driver;
	toy_vulkan_asset_loader_t vk_asset_loader;
	toy_vulkan_mesh_primitive_asset_pool_t vk_mesh_primitive_pool;
}toy_asset_manager_vulkan_private_t;

static void toy_destroy_vulkan_mesh_primitive (
	toy_asset_pool_t* pool,
	void* asset)
{
	toy_vulkan_mesh_primitive_asset_pool_t* vk_primitive_pool = pool->context;
	toy_vulkan_mesh_primitive_p mesh_primitive = asset;

	toy_free_vulkan_mesh_primitive(vk_primitive_pool, mesh_primitive);
}

static void toy_destroy_mesh (
	toy_asset_pool_t* pool,
	void* asset)
{
	toy_asset_item_ref_pool_t* ref_pool = pool->context;
	toy_mesh_t* mesh = asset;

	toy_asset_pool_item_ref_t* ref = &mesh->first_primitive;
	do {
		toy_asset_pool_item_ref_t* next_ref = toy_get_next_asset_item_ref(ref_pool, ref);
		toy_sub_asset_ref(ref->pool, ref->index, 1);
		if (0 == toy_get_asset_ref(ref->pool, ref->index))
			toy_free_asset_item(ref->pool, ref->index);
		ref = next_ref;
	} while (NULL != ref);
}


void toy_create_asset_manager (
	size_t cache_size,
	toy_memory_allocator_t* alc,
	void* render_driver,
	toy_asset_manager_t* output,
	toy_error_t* error)
{
	TOY_ASSERT(1024 * 1024 <= cache_size && "Too small cache size");
	TOY_ASSERT(NULL != output && NULL != error);

	memset(output, 0, sizeof(*output));

	output->alc = alc;

	output->cache_stack = toy_create_memory_stack(cache_size + sizeof(toy_memory_stack_t), alc, &output->stack_alc_L, &output->stack_alc_R);
	if (NULL == output->cache_stack) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Failed to create asset cache stack", error);
		goto FAIL_MEMORY_STACK;
	}
	output->cache_size = cache_size;

	output->file_api = toy_std_file_interface();

	toy_create_memory_pools(8 * 1024 * 1024, TOY_MEMORY_CHUNK_SIZE, alc, &output->chunk_pools, &output->chunk_alc);
	if (NULL == output->chunk_pools) {
		toy_err(TOY_ERROR_MEMORY_DEVICE_ALLOCATION_FAILED, "Failed to create asset chunk pool", error);
		goto FAIL_MEMORY_POOL;
	}

	toy_create_asset_item_ref_pool(256, &alc->buddy_alc, &output->item_ref_pool, error);
	if (toy_is_failed(*error))
		goto FAIL_ITEM_REF_POOL;

	toy_vulkan_driver_t* vk_driver = render_driver;
	toy_asset_manager_vulkan_private_t* vk_private = toy_alloc_aligned(
		&alc->buddy_alc, sizeof(toy_asset_manager_vulkan_private_t), sizeof(void*));
	if (NULL == vk_private) {
		toy_err(TOY_ERROR_MEMORY_HOST_ALLOCATION_FAILED, "Failed to alloc asset manager private data", error);
		goto FAIL_PRIVATE;
	}
	vk_private->vk_driver = render_driver;
	output->mgr_private = vk_private;

	toy_create_vulkan_asset_loader(
		&vk_driver->device,
		16 * 1024 * 1024,
		&vk_driver->vk_allocator,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_VK_ASSET_LOADER;

	toy_create_vulkan_mesh_primitive_asset_pool(
		2 * 1024 * 1024,
		1024 * 1024,
		&vk_driver->vk_allocator,
		&vk_private->vk_mesh_primitive_pool,
		error);
	if (toy_is_failed(*error))
		goto FAIL_VK_MESH_PRIMITIVE_ASSET_POOL;

	toy_init_asset_pool(
		sizeof(toy_vulkan_mesh_primitive_t),
		sizeof(void*),
		toy_destroy_vulkan_mesh_primitive,
		&output->chunk_alc,
		&alc->buddy_alc,
		&vk_private->vk_mesh_primitive_pool,
		"Vulkan mesh primitive",
		&output->asset_pools.mesh_primitive);

	toy_init_asset_pool(
		sizeof(toy_mesh_t),
		sizeof(void*),
		toy_destroy_mesh,
		&output->chunk_alc,
		&alc->buddy_alc,
		&output->item_ref_pool,
		"Mesh",
		&output->asset_pools.mesh);
	
	toy_ok(error);
	return;

FAIL_VK_MESH_PRIMITIVE_ASSET_POOL:
	toy_destroy_vulkan_asset_loader(vk_driver->device.handle, &vk_private->vk_asset_loader);
FAIL_VK_ASSET_LOADER:
	toy_free_aligned(&alc->buddy_alc, vk_private);
FAIL_PRIVATE:
	toy_destroy_asset_ref_pool(&output->item_ref_pool);
FAIL_ITEM_REF_POOL:
	toy_destroy_memory_pools(output->chunk_pools, alc);
FAIL_MEMORY_POOL:
	toy_destroy_memory_stack(output->cache_stack, alc);
FAIL_MEMORY_STACK:
	return;
}


void toy_destroy_asset_manager (
	toy_asset_manager_t* asset_mgr)
{
	toy_memory_allocator_t* alc = asset_mgr->alc;
	toy_asset_manager_vulkan_private_t* vk_private = asset_mgr->mgr_private;

	toy_destroy_asset_pool(&alc->buddy_alc, &asset_mgr->asset_pools.mesh_primitive);

	toy_destroy_vulkan_mesh_primitive_asset_pool(&vk_private->vk_mesh_primitive_pool);
	toy_destroy_vulkan_asset_loader(vk_private->vk_driver->device.handle, &vk_private->vk_asset_loader);

	toy_free_aligned(&alc->buddy_alc, asset_mgr->mgr_private);

	toy_destroy_asset_ref_pool(&asset_mgr->item_ref_pool);
	toy_destroy_memory_pools(asset_mgr->chunk_pools, asset_mgr->alc);
	toy_destroy_memory_stack(asset_mgr->cache_stack, asset_mgr->alc);
}



static void toy_alloc_mesh_primitive_item (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive_data,
	toy_asset_pool_item_ref_t* output,
	toy_error_t* error)
{
	toy_asset_manager_vulkan_private_t* vk_private = asset_mgr->mgr_private;
	// Alloc asset item
	uint32_t primitive_ref_index = toy_alloc_asset_item(
		&asset_mgr->asset_pools.mesh_primitive,
		error);
	if (toy_is_failed(*error))
		goto FAIL_ALLOC_REF;

	toy_vulkan_mesh_primitive_t* vk_primitive = toy_get_asset_item(
		&asset_mgr->asset_pools.mesh_primitive,
		primitive_ref_index);

	toy_alloc_vulkan_mesh_primitive(
		&vk_private->vk_mesh_primitive_pool,
		primitive_data->attribute_size,
		primitive_data->index_size,
		vk_primitive,
		error);
	if (toy_is_failed(*error))
		goto FAIL_ALLOC_MESH_PRIMITIVE;

	output->pool = &asset_mgr->asset_pools.mesh_primitive;
	output->index = primitive_ref_index;
	output->next_ref = UINT32_MAX;
	toy_ok(error);
	return;

FAIL_ALLOC_MESH_PRIMITIVE:
	toy_raw_free_asset_item(&asset_mgr->asset_pools.mesh_primitive, primitive_ref_index);
FAIL_ALLOC_REF:
	return;
}


void toy_load_mesh_primitive (
	toy_asset_manager_t* asset_mgr,
	const toy_host_mesh_primitive_t* primitive_data,
	toy_asset_pool_item_ref_t* output,
	toy_error_t* error)
{
	toy_asset_manager_vulkan_private_t* vk_private = asset_mgr->mgr_private;
	VkResult vk_err;

	// Alloc asset item
	toy_alloc_mesh_primitive_item(
		asset_mgr, primitive_data, output, error);
	if (toy_is_failed(*error))
		goto FAIL_ALLOC_ITEM;

	toy_vulkan_mesh_primitive_t* vk_primitive = toy_get_asset_item(output->pool, output->index);

	toy_reset_vulkan_asset_loader(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_RESET_LOADER;

	// Copy data to stage memory
	toy_stage_data_block_t data_blocks[2];
	toy_vulkan_sub_buffer_t stage_sub_buffers[2];
	data_blocks[0].data = primitive_data->attributes;
	data_blocks[0].size = primitive_data->attribute_size;
	data_blocks[0].alignment = sizeof(float);
	data_blocks[1].data = primitive_data->indices;
	data_blocks[1].size = primitive_data->index_size;
	data_blocks[1].alignment = primitive_data->index_count > UINT16_MAX ? sizeof(uint32_t) : sizeof(uint16_t);

	toy_map_vulkan_stage_memory(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_MAP_STAGE_MEMORY;

	toy_copy_data_to_vulkan_stage_memory(
		data_blocks,
		NULL != primitive_data->indices ? 2 : 1,
		&vk_private->vk_asset_loader,
		stage_sub_buffers,
		error);
	if (toy_is_failed(*error))
		goto FAIL_COPY_TO_STAGE_MEMORY;

	toy_unmap_vulkan_stage_memory(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_UNMAP_STAGE_MEMORY;

	// Copy stage memory to gpu local memory
	vk_err = toy_start_copy_stage_mesh_primitive_data_cmd(
		&vk_private->vk_asset_loader);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Failed to record vkcmd of loading mesh primitive", error);
		goto FAIL_START_CMD;
	}

	toy_vkcmd_copy_stage_mesh_primitive_data(
		&vk_private->vk_asset_loader,
		&stage_sub_buffers[0],
		NULL != primitive_data->indices ? &stage_sub_buffers[1] : NULL,
		vk_primitive);

	// Post process commands, for mesh primitive, it's nothing

	toy_submit_vulkan_mesh_primitive_loading_cmd(
		vk_private->vk_driver->device.handle,
		&vk_private->vk_asset_loader,
		error);
	if (toy_is_failed(*error))
		goto FAIL_SUBMIT_CMD;

	vk_err = vkWaitForFences(
		vk_private->vk_driver->device.handle,
		1,
		&vk_private->vk_asset_loader.fence,
		VK_TRUE,
		UINT64_MAX);
	if (VK_SUCCESS != vk_err) {
		toy_err_vkerr(TOY_ERROR_OPERATION_FAILED, vk_err, "Wait for asset loading failed", error);
		goto FAIL_WAIT_SUBMIT;
	}

	// Fill others
	vk_primitive->vertex_count = primitive_data->vertex_count;
	vk_primitive->index_count = primitive_data->index_count;
	vk_primitive->index_type = primitive_data->index_count > UINT16_MAX ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

	toy_ok(error);
	return;

FAIL_WAIT_SUBMIT:
FAIL_SUBMIT_CMD:
FAIL_START_CMD:
FAIL_UNMAP_STAGE_MEMORY:
	toy_clear_vulkan_stage_memory(&vk_private->vk_asset_loader);
FAIL_COPY_TO_STAGE_MEMORY:
FAIL_MAP_STAGE_MEMORY:
FAIL_RESET_LOADER:
FAIL_ALLOC_ITEM:
	toy_free_asset_item(output->pool, output->index);
	return;
}
